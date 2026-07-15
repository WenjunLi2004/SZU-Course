import warnings

import joblib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.interpolate import lagrange
from sklearn.metrics import accuracy_score, auc, confusion_matrix, roc_curve
from sklearn.neural_network import MLPClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.utils import shuffle


def ployinterp_column(s, n, k=5):
    """使用缺失值前后最多k个非空数据进行拉格朗日插值。"""
    y = s[list(range(max(n - k, 0), n)) + list(range(n + 1, min(n + 1 + k, len(s))))]
    y = y[y.notnull()]
    return lagrange(y.index, list(y))(n)


def fill_missing_data(inputfile="missing_data.xls", outputfile="missing_data_processed.xlsx"):
    data = pd.read_excel(inputfile, header=None)
    print("原始缺失值数量：")
    print(data.isna().sum())
    print("\n原始数据统计：")
    print(data.describe())

    for i in data.columns:
        for j in range(len(data)):
            if pd.isna(data.loc[j, i]):
                data.loc[j, i] = ployinterp_column(data[i], j)

    print("\n插值后缺失值数量：")
    print(data.isna().sum())
    data.to_excel(outputfile, header=None, index=False)
    return data


def plot_confusion_matrix(cm, title, outputfile):
    fig, ax = plt.subplots(figsize=(4, 3.5), dpi=150)
    ax.imshow(cm, cmap="Blues")
    ax.set_xticks([0, 1])
    ax.set_yticks([0, 1])
    ax.set_xticklabels(["0", "1"])
    ax.set_yticklabels(["0", "1"])
    ax.set_xlabel("Predicted")
    ax.set_ylabel("True")
    ax.set_title(title)
    for y in range(cm.shape[0]):
        for x in range(cm.shape[1]):
            ax.annotate(str(cm[y, x]), xy=(x, y), ha="center", va="center")
    fig.tight_layout()
    fig.savefig(outputfile)
    plt.close(fig)


def plot_roc_curve(fpr, tpr, auc_value, title, outputfile, color):
    fig, ax = plt.subplots(figsize=(4, 3.5), dpi=150)
    ax.plot(fpr, tpr, color=color, linewidth=2, label=f"AUC={auc_value:.4f}")
    ax.plot([0, 1], [0, 1], "k--", linewidth=1)
    ax.set_xlabel("False Positive Rate")
    ax.set_ylabel("True Positive Rate")
    ax.set_title(title)
    ax.legend(loc="lower right")
    fig.tight_layout()
    fig.savefig(outputfile)
    plt.close(fig)


def train_and_evaluate_models(inputfile="model.xls"):
    data = pd.read_excel(inputfile)
    print("\n专家样本数据规模：", data.shape)
    print("标签分布：")
    print(data.iloc[:, -1].value_counts())

    data = shuffle(data.values, random_state=0)
    train_size = int(len(data) * 0.8)
    train = data[:train_size, :].astype(float)
    test = data[train_size:, :].astype(float)
    print("\n训练集规模：", train.shape)
    print("测试集规模：", test.shape)

    x_train, y_train = train[:, :3], train[:, 3]
    x_test, y_test = test[:, :3], test[:, 3]

    tree = DecisionTreeClassifier(random_state=0)
    tree.fit(x_train, y_train)
    tree_pred = tree.predict(x_test)
    tree_prob = tree.predict_proba(x_test)[:, list(tree.classes_).index(1.0)]
    tree_cm = confusion_matrix(y_test, tree_pred, labels=[0, 1])
    tree_fpr, tree_tpr, _ = roc_curve(y_test, tree_prob, pos_label=1)
    tree_auc = auc(tree_fpr, tree_tpr)
    joblib.dump(tree, "tree.pkl")

 t

    print("\nCART决策树评价结果：")
    print("训练集准确率：", round(accuracy_score(y_train, tree.predict(x_train)), 4))
    print("测试集准确率：", round(accuracy_score(y_test, tree_pred), 4))
    print("混淆矩阵：")
    print(tree_cm)
    print("ROC AUC：", round(tree_auc, 4))

    print("\nLM神经网络评价结果：")
    print("训练集准确率：", round(accuracy_score(y_train, net.predict(x_train)), 4))
    print("测试集准确率：", round(accuracy_score(y_test, net_pred), 4))
    print("混淆矩阵：")
    print(net_cm)
    print("ROC AUC：", round(net_auc, 4))

    plot_confusion_matrix(tree_cm, "CART confusion matrix", "cart_confusion_matrix.png")
    plot_roc_curve(tree_fpr, tree_tpr, tree_auc, "CART ROC curve", "cart_roc.png", "green")
    plot_confusion_matrix(net_cm, "LM neural network confusion matrix", "mlp_confusion_matrix.png")
    plot_roc_curve(net_fpr, net_tpr, net_auc, "LM neural network ROC curve", "mlp_roc.png", "blue")


if __name__ == "__main__":
    fill_missing_data()
    train_and_evaluate_models()
