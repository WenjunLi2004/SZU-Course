"""Kaggle House Prices — feature engineering + ensemble regression.

Reproduces the full pipeline described in 实验手册：房价预测.pdf:
    outlier removal → log/Box-Cox transform → missing value imputation →
    feature combination → label/dummy encoding → PCA exploration →
    linear/regularized regression + bagging/boosting + stacking → submission.
"""

from __future__ import annotations

import os
import warnings

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from scipy import stats
from scipy.special import boxcox1p
from scipy.stats import norm, skew
from sklearn.base import BaseEstimator, RegressorMixin, TransformerMixin, clone
from sklearn.decomposition import PCA
from sklearn.ensemble import (
    AdaBoostRegressor,
    BaggingRegressor,
    GradientBoostingRegressor,
    RandomForestRegressor,
)
from sklearn.kernel_ridge import KernelRidge
from sklearn.linear_model import ElasticNet, Lasso, LinearRegression, Ridge
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import KFold, cross_val_score
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import LabelEncoder, RobustScaler

import lightgbm as lgb
import xgboost as xgb

warnings.filterwarnings("ignore")

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "数据集")
OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
FIG_DIR = os.path.join(OUTPUT_DIR, "figures")
os.makedirs(FIG_DIR, exist_ok=True)

sns.set_style("darkgrid")
RNG = 42


# ---------------------------------------------------------------- load data
def load_data():
    train = pd.read_csv(os.path.join(DATA_DIR, "train.csv"))
    test = pd.read_csv(os.path.join(DATA_DIR, "test.csv"))
    print(f"train shape before drop Id: {train.shape}")
    print(f"test  shape before drop Id: {test.shape}")
    train_id = train["Id"]
    test_id = test["Id"]
    train.drop("Id", axis=1, inplace=True)
    test.drop("Id", axis=1, inplace=True)
    return train, test, train_id, test_id


# --------------------------------------------------------------- preprocess
def remove_outliers(train: pd.DataFrame) -> pd.DataFrame:
    fig, ax = plt.subplots()
    ax.scatter(train["GrLivArea"], train["SalePrice"])
    plt.xlabel("GrLivArea")
    plt.ylabel("SalePrice")
    plt.title("Before outlier removal")
    fig.savefig(os.path.join(FIG_DIR, "outliers_before.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)

    train = train.drop(
        train[(train["GrLivArea"] > 4000) & (train["SalePrice"] < 300000)].index
    )

    fig, ax = plt.subplots()
    ax.scatter(train["GrLivArea"], train["SalePrice"])
    plt.xlabel("GrLivArea")
    plt.ylabel("SalePrice")
    plt.title("After outlier removal")
    fig.savefig(os.path.join(FIG_DIR, "outliers_after.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)
    return train


def plot_target_distribution(series: pd.Series, suffix: str) -> tuple[float, float]:
    mu, sigma = norm.fit(series)
    fig = plt.figure()
    sns.histplot(series, kde=True, stat="density")
    xs = np.linspace(series.min(), series.max(), 200)
    plt.plot(xs, norm.pdf(xs, mu, sigma), "k", label=f"N(mu={mu:.2f},sigma={sigma:.2f})")
    plt.legend()
    plt.ylabel("Frequency")
    plt.title(f"SalePrice distribution ({suffix})")
    fig.savefig(os.path.join(FIG_DIR, f"saleprice_dist_{suffix}.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)

    fig = plt.figure()
    stats.probplot(series, plot=plt)
    plt.title(f"SalePrice QQ plot ({suffix})")
    fig.savefig(os.path.join(FIG_DIR, f"saleprice_qq_{suffix}.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)
    return mu, sigma


def fill_missing(all_data: pd.DataFrame) -> pd.DataFrame:
    none_cols = [
        "PoolQC", "MiscFeature", "Alley", "Fence", "FireplaceQu",
        "GarageType", "GarageFinish", "GarageQual", "GarageCond",
        "BsmtQual", "BsmtCond", "BsmtExposure", "BsmtFinType1", "BsmtFinType2",
        "MasVnrType", "MSSubClass",
    ]
    for c in none_cols:
        all_data[c] = all_data[c].fillna("None")

    zero_cols = [
        "GarageYrBlt", "GarageArea", "GarageCars",
        "BsmtFinSF1", "BsmtFinSF2", "BsmtUnfSF", "TotalBsmtSF",
        "BsmtFullBath", "BsmtHalfBath", "MasVnrArea",
    ]
    for c in zero_cols:
        all_data[c] = all_data[c].fillna(0)

    all_data["LotFrontage"] = all_data.groupby("Neighborhood")["LotFrontage"].transform(
        lambda x: x.fillna(x.median())
    )
    all_data["MSZoning"] = all_data["MSZoning"].fillna(all_data["MSZoning"].mode()[0])
    all_data = all_data.drop(["Utilities"], axis=1)
    all_data["Functional"] = all_data["Functional"].fillna("Typ")
    for c in ("Electrical", "KitchenQual", "Exterior1st", "Exterior2nd", "SaleType"):
        all_data[c] = all_data[c].fillna(all_data[c].mode()[0])
    return all_data


def plot_missing_ratio(all_data: pd.DataFrame, filename: str) -> pd.Series:
    ratio = (all_data.isnull().sum() / len(all_data)) * 100
    ratio = ratio[ratio > 0].sort_values(ascending=False)
    if ratio.empty:
        return ratio
    fig, ax = plt.subplots(figsize=(12, 6))
    sns.barplot(x=ratio.index, y=ratio.values, ax=ax)
    plt.xticks(rotation=90)
    plt.xlabel("Features")
    plt.ylabel("Percent of missing values")
    plt.title("Percent missing data by feature")
    fig.savefig(os.path.join(FIG_DIR, filename), dpi=130, bbox_inches="tight")
    plt.close(fig)
    return ratio


def label_encode(all_data: pd.DataFrame) -> pd.DataFrame:
    all_data["MSSubClass"] = all_data["MSSubClass"].apply(str)
    all_data["OverallCond"] = all_data["OverallCond"].astype(str)
    all_data["YrSold"] = all_data["YrSold"].astype(str)
    all_data["MoSold"] = all_data["MoSold"].astype(str)

    cols = (
        "FireplaceQu", "BsmtQual", "BsmtCond", "GarageQual", "GarageCond",
        "ExterQual", "ExterCond", "HeatingQC", "PoolQC", "KitchenQual",
        "BsmtFinType1", "BsmtFinType2", "Functional", "Fence", "BsmtExposure",
        "GarageFinish", "LandSlope", "LotShape", "PavedDrive", "Street", "Alley",
        "CentralAir", "MSSubClass", "OverallCond", "YrSold", "MoSold",
    )
    for c in cols:
        lbl = LabelEncoder()
        lbl.fit(list(all_data[c].values))
        all_data[c] = lbl.transform(list(all_data[c].values))
    return all_data


def plot_corr_heatmap(train_with_target: pd.DataFrame) -> None:
    corr = train_with_target.select_dtypes(include=[np.number]).corr()
    fig, _ = plt.subplots(figsize=(14, 11))
    sns.heatmap(corr, vmax=0.9, square=True, cmap="rocket_r")
    plt.title("Feature correlation heatmap")
    fig.savefig(os.path.join(FIG_DIR, "corr_heatmap.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)


# ----------------------------------------------------------------- modeling
class AveragingModels(BaseEstimator, RegressorMixin, TransformerMixin):
    def __init__(self, models):
        self.models = models

    def fit(self, X, y):
        self.models_ = [clone(m) for m in self.models]
        for m in self.models_:
            m.fit(X, y)
        return self

    def predict(self, X):
        preds = np.column_stack([m.predict(X) for m in self.models_])
        return np.mean(preds, axis=1)


class StackingAveragedModels(BaseEstimator, RegressorMixin, TransformerMixin):
    def __init__(self, base_models, meta_model, n_folds=5):
        self.base_models = base_models
        self.meta_model = meta_model
        self.n_folds = n_folds

    def fit(self, X, y):
        self.base_models_ = [list() for _ in self.base_models]
        self.meta_model_ = clone(self.meta_model)
        kfold = KFold(n_splits=self.n_folds, shuffle=True, random_state=156)
        out_of_fold = np.zeros((X.shape[0], len(self.base_models)))
        for i, model in enumerate(self.base_models):
            for tr_idx, ho_idx in kfold.split(X, y):
                inst = clone(model)
                self.base_models_[i].append(inst)
                inst.fit(X[tr_idx], y[tr_idx])
                out_of_fold[ho_idx, i] = inst.predict(X[ho_idx])
        self.meta_model_.fit(out_of_fold, y)
        return self

    def predict(self, X):
        meta_features = np.column_stack([
            np.column_stack([m.predict(X) for m in base_models]).mean(axis=1)
            for base_models in self.base_models_
        ])
        return self.meta_model_.predict(meta_features)


def rmsle_cv(model, X, y, n_folds=5):
    kf = KFold(n_splits=n_folds, shuffle=True, random_state=RNG).get_n_splits(X)
    rmse = np.sqrt(-cross_val_score(
        model, X, y, scoring="neg_mean_squared_error", cv=kf,
    ))
    return rmse


def rmsle(y, y_pred):
    return np.sqrt(mean_squared_error(y, y_pred))


# ---------------------------------------------------------------------- main
def main():
    train, test, train_id, test_id = load_data()

    train = remove_outliers(train)
    plot_target_distribution(train["SalePrice"], "raw")
    train["SalePrice"] = np.log1p(train["SalePrice"])
    plot_target_distribution(train["SalePrice"], "log")

    ntrain = train.shape[0]
    y_train = train["SalePrice"].values
    all_data = pd.concat((train, test)).reset_index(drop=True)
    all_data.drop(["SalePrice"], axis=1, inplace=True)
    print(f"all_data size: {all_data.shape}")

    missing_before = plot_missing_ratio(all_data, "missing_before.png")
    print("\nTop missing features (%):")
    print(missing_before.head(20))

    all_data = fill_missing(all_data)
    remaining = (all_data.isnull().sum() / len(all_data)) * 100
    remaining = remaining[remaining > 0].sort_values(ascending=False)
    print(f"\nMissing features after fill: {len(remaining)}")

    numeric_feats = all_data.select_dtypes(include=[np.number]).columns
    skewed = all_data[numeric_feats].apply(lambda x: skew(x.dropna())).sort_values(ascending=False)
    skewness = skewed[abs(skewed) > 0.75]
    print(f"\nBox-Cox transforming {skewness.shape[0]} skewed features")
    print(skewness.head(10))
    lam = 0.15
    for feat in skewness.index:
        all_data[feat] = boxcox1p(all_data[feat], lam)

    all_data["TotalSF"] = (
        all_data["TotalBsmtSF"] + all_data["1stFlrSF"] + all_data["2ndFlrSF"]
    )

    all_data = label_encode(all_data)
    all_data = pd.get_dummies(all_data)
    print(f"\nFeature matrix after dummy encoding: {all_data.shape}")

    pca = PCA(n_components=100, random_state=RNG)
    all_data_pca = pca.fit_transform(all_data)
    print(f"PCA shape: {all_data_pca.shape}, explained variance ratio sum: "
          f"{pca.explained_variance_ratio_.sum():.4f}")

    train_x = all_data[:ntrain].values
    test_x = all_data[ntrain:].values

    train_with_target = all_data[:ntrain].copy()
    train_with_target["SalePrice"] = y_train
    plot_corr_heatmap(train_with_target)

    linear = make_pipeline(RobustScaler(), LinearRegression())
    ridge = make_pipeline(RobustScaler(), Ridge(alpha=0.9, random_state=1))
    lasso = make_pipeline(RobustScaler(), Lasso(alpha=0.0005, random_state=1))
    ENet = make_pipeline(RobustScaler(), ElasticNet(alpha=0.0005, l1_ratio=0.9, random_state=1))
    KRR = KernelRidge(alpha=0.6, kernel="polynomial", degree=2, coef0=2.5)

    Bagging = BaggingRegressor(estimator=LinearRegression(), n_estimators=10, random_state=1)
    RandomForest = RandomForestRegressor(n_estimators=100, random_state=RNG)
    AdaBoost = AdaBoostRegressor(estimator=LinearRegression(), n_estimators=50,
                                 learning_rate=1.0, loss="linear", random_state=RNG)
    GBoost = GradientBoostingRegressor(
        n_estimators=3000, learning_rate=0.05, max_depth=4, max_features="sqrt",
        min_samples_leaf=15, min_samples_split=10, loss="huber", random_state=5,
    )
    model_xgb = xgb.XGBRegressor(
        colsample_bytree=0.4603, gamma=0.0468, learning_rate=0.05, max_depth=3,
        min_child_weight=1.7817, n_estimators=2200, reg_alpha=0.4640,
        reg_lambda=0.8571, subsample=0.5213, random_state=7, n_jobs=-1, verbosity=0,
    )
    model_lgb = lgb.LGBMRegressor(
        objective="regression", num_leaves=5, learning_rate=0.05, n_estimators=720,
        max_bin=55, bagging_fraction=0.8, bagging_freq=5, feature_fraction=0.2319,
        feature_fraction_seed=9, bagging_seed=9, min_data_in_leaf=6,
        min_sum_hessian_in_leaf=11, verbose=-1,
    )

    scores: dict[str, tuple[float, float]] = {}

    def report(name, model):
        s = rmsle_cv(model, train_x, y_train)
        scores[name] = (s.mean(), s.std())
        print(f"{name:>22s}  RMSE = {s.mean():.4f} ({s.std():.4f})")

    print("\n=== Single regression models ===")
    for name, m in [
        ("LinearRegression", linear), ("Ridge", ridge), ("Lasso", lasso),
        ("ElasticNet", ENet), ("KernelRidge", KRR),
    ]:
        report(name, m)

    print("\n=== Bagging models ===")
    for name, m in [("Bagging(LR)", Bagging), ("RandomForest", RandomForest)]:
        report(name, m)

    print("\n=== Boosting models ===")
    for name, m in [
        ("AdaBoost(LR)", AdaBoost), ("GradientBoosting", GBoost),
        ("XGBoost", model_xgb), ("LightGBM", model_lgb),
    ]:
        report(name, m)

    print("\n=== Stacking (averaging) ===")
    averaged = AveragingModels(models=(ENet, GBoost, KRR, lasso))
    report("AveragedBaseModels", averaged)

    stacked = StackingAveragedModels(
        base_models=(ENet, GBoost, KRR), meta_model=lasso,
    )
    report("StackingAveraged", stacked)

    fig, ax = plt.subplots(figsize=(10, 6))
    names = list(scores.keys())
    means = [scores[n][0] for n in names]
    stds = [scores[n][1] for n in names]
    ax.barh(names, means, xerr=stds, color="steelblue")
    ax.set_xlabel("5-fold CV RMSE (log SalePrice)")
    ax.invert_yaxis()
    ax.set_title("Model comparison")
    for i, v in enumerate(means):
        ax.text(v + 0.001, i, f"{v:.4f}", va="center")
    fig.savefig(os.path.join(FIG_DIR, "model_scores.png"), dpi=130, bbox_inches="tight")
    plt.close(fig)

    print("\n=== Final weighted ensemble: 0.70 Stacking + 0.15 XGBoost + 0.15 LightGBM ===")
    stacked.fit(train_x, y_train)
    stacked_train_pred = stacked.predict(train_x)
    stacked_pred = np.expm1(stacked.predict(test_x))
    print(f"Stacking train RMSE: {rmsle(y_train, stacked_train_pred):.4f}")

    model_xgb.fit(train_x, y_train)
    xgb_train_pred = model_xgb.predict(train_x)
    xgb_pred = np.expm1(model_xgb.predict(test_x))
    print(f"XGBoost  train RMSE: {rmsle(y_train, xgb_train_pred):.4f}")

    model_lgb.fit(train_x, y_train)
    lgb_train_pred = model_lgb.predict(train_x)
    lgb_pred = np.expm1(model_lgb.predict(test_x))
    print(f"LightGBM train RMSE: {rmsle(y_train, lgb_train_pred):.4f}")

    blended_train = (
        0.70 * stacked_train_pred + 0.15 * xgb_train_pred + 0.15 * lgb_train_pred
    )
    final_train_rmse = rmsle(y_train, blended_train)
    print(f"Weighted blend train RMSE: {final_train_rmse:.4f}")

    ensemble = 0.70 * stacked_pred + 0.15 * xgb_pred + 0.15 * lgb_pred
    sub = pd.DataFrame({"Id": test_id, "SalePrice": ensemble})
    sub_path = os.path.join(OUTPUT_DIR, "submission.csv")
    sub.to_csv(sub_path, index=False)
    print(f"\nSubmission written: {sub_path}")

    summary_path = os.path.join(OUTPUT_DIR, "metrics.txt")
    with open(summary_path, "w") as f:
        f.write("=== 5-fold CV RMSE (log SalePrice) ===\n")
        for n in names:
            f.write(f"{n:>22s}  {scores[n][0]:.4f} ({scores[n][1]:.4f})\n")
        f.write("\n=== Final ensemble train RMSE ===\n")
        f.write(f"Stacking train RMSE : {rmsle(y_train, stacked_train_pred):.4f}\n")
        f.write(f"XGBoost  train RMSE : {rmsle(y_train, xgb_train_pred):.4f}\n")
        f.write(f"LightGBM train RMSE : {rmsle(y_train, lgb_train_pred):.4f}\n")
        f.write(f"Weighted blend RMSE : {final_train_rmse:.4f}\n")
        f.write(f"\nSubmission rows: {len(sub)}\n")
    print(f"Metrics summary written: {summary_path}")


if __name__ == "__main__":
    main()
