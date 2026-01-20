'use strict';

var obsidian = require('obsidian');

/******************************************************************************
Copyright (c) Microsoft Corporation.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
***************************************************************************** */
/* global Reflect, Promise, SuppressedError, Symbol */

var extendStatics = function(d, b) {
    extendStatics = Object.setPrototypeOf ||
        ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
        function (d, b) { for (var p in b) if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p]; };
    return extendStatics(d, b);
};

function __extends(d, b) {
    if (typeof b !== "function" && b !== null)
        throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
    extendStatics(d, b);
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
}

typeof SuppressedError === "function" ? SuppressedError : function (error, suppressed, message) {
    var e = new Error(message);
    return e.name = "SuppressedError", e.error = error, e.suppressed = suppressed, e;
};

function createCommonjsModule(fn, basedir, module) {
	return module = {
		path: basedir,
		exports: {},
		require: function (path, base) {
			return commonjsRequire(path, (base === undefined || base === null) ? module.path : base);
		}
	}, fn(module, module.exports), module.exports;
}

function commonjsRequire () {
	throw new Error('Dynamic requires are not currently supported by @rollup/plugin-commonjs');
}

var codemirror = CodeMirror;

createCommonjsModule(function (module, exports) {
(function(mod) {
  mod(codemirror);
})(function(CodeMirror) {

  function wordSet(words) {
    var set = {};
    for (var i = 0; i < words.length; i++) set[words[i]] = true;
    return set
  }

  var keywords = wordSet(["module", "func"]);
  var types = wordSet(["f32"]);
  var buildin = wordSet(["call"]);
  var rankedTypes = wordSet(["tensor", "memref"]);
  var punc = ":;,.(){}[]<>";
  var hexadecimal = /^\-?0x[\dA-Fa-f][\dA-Fa-f_]*(?:(?:\.[\dA-Fa-f][\dA-Fa-f_]*)?[Pp]\-?\d[\d_]*)?/;
  var decimal = /^\-?\d[\d_]*(?:\.\d[\d_]*)?(?:[Ee]\-?\d[\d_]*)?/;
  var float = /[-+]?[0-9]+[.][0-9]*([eE][-+]?[0-9]+)?/;


  var bareIdentifier = /([a-zA-Z$._-]|[_])([a-zA-Z$._-]|[0-9]|[_$.])*/;
  var suffixIdentifier = /([0-9]+|([a-zA-Z$._-][0-9a-zA-Z$._-]*))/;

  function tokenBase(stream, state, prev) {
    if (stream.sol()) state.indented = stream.indentation();
    if (stream.eatSpace()) return null

    var ch = stream.peek();
    if (ch == "/") {
      if (stream.match("//")) {
        stream.skipToEnd();
        return "comment"
      }
    }
    if (stream.match(decimal)) return "number"
    if (stream.match(hexadecimal)) return "number"
    if (stream.match(float)) return "number"
    if (ch == "%") {
      stream.next();
      if (stream.match(suffixIdentifier)) {
        return "variable"
      }
    }
    if (ch == "@") {
      stream.next();
      if (stream.match(suffixIdentifier)) {
        return "variable-2"
      }
    }
    if (punc.indexOf(ch) > -1) {
      stream.next();
      return "punctuation"
    }

    if (stream.match(bareIdentifier)) {
      var ident = stream.current();
      if (types.hasOwnProperty(ident)) return "variable-3"
      if (rankedTypes.hasOwnProperty(ident)) {
        var ch = stream.peek();
        if (ch == "<") {
          var tokenize = tokenTypeRank;
          state.tokenize.push(tokenize);
        }
        return "variable-3"
      }
      if (buildin.hasOwnProperty(ident)) return "builtin"
      if (keywords.hasOwnProperty(ident)) return "keyword"
    }

    stream.next();
    return null
  }

  function tokenTypeRank(stream, state, prev) {
    var ch = stream.peek();
    while (true) {
      if (ch == ">") {
        stream.next();
        state.tokenize.pop();
        return null
      }
      if (ch == "x") {
        stream.next();
        return "builtin"
      }
      if (ch == "?") {
        stream.next();
        return "keyword"
      }
      if (stream.match(decimal)) {
        return "number"
      }

      stream.next();
      ch = stream.peek();
    }
  }

  function Context(prev, align, indented) {
    this.prev = prev;
    this.align = align;
    this.indented = indented;
  }

  function pushContext(state, stream) {
    var align = stream.match(/^\s*($|\/[\/\*])/, false) ? null : stream.column() + 1;
    state.context = new Context(state.context, align, state.indented);
  }

  function popContext(state) {
    if (state.context) {
      state.indented = state.context.indented;
      state.context = state.context.prev;
    }
  }

  CodeMirror.defineMode("mlir", function(config) {
    return {
      startState: function() {
        return {
          prev: null,
          context: null,
          indented: 0,
          tokenize: []
        }
      },

      token: function(stream, state) {
        var prev = state.prev;
        state.prev = null;
        var tokenize = state.tokenize[state.tokenize.length - 1] || tokenBase;
        var style = tokenize(stream, state, prev);
        if (!style || style == "comment") state.prev = prev;
        else if (!state.prev) state.prev = style;

        if (style == "punctuation") {
          var bracket = /[\(\[\{]|([\]\)\}])/.exec(stream.current());
          if (bracket) {
            (bracket[1] ? popContext : pushContext)(state, stream);
          }
        }

        return style
      },

      indent: function(state, textAfter) {
        var cx = state.context;
        if (!cx) return 0
        var closing = /^[\]\}\)]/.test(textAfter);
        if (cx.align != null) return cx.align - (closing ? 1 : 0)
        return cx.indented + (closing ? 0 : config.indentUnit)
      },

      electricInput: /^\s*[\)\}\]]$/,

      lineComment: "//",
      blockCommentStart: "/*",
      blockCommentEnd: "*/",
      fold: "brace",
      closeBrackets: "()[]{}''\"\"``"
    }
  });

  CodeMirror.defineMIME("text/x-mlir","mlir");
});
});

var MLIRSyntaxHighlightPlugin = /** @class */ (function (_super) {
    __extends(MLIRSyntaxHighlightPlugin, _super);
    function MLIRSyntaxHighlightPlugin() {
        var _this = _super !== null && _super.apply(this, arguments) || this;
        // these are the CodeMirror modes that Obsidian uses by default
        _this.modesToKeep = ["hypermd", "markdown", "null", "xml"];
        _this.refreshLeaves = function () {
            // re-set the editor mode to refresh the syntax highlighting
            _this.app.workspace.iterateCodeMirrors(function (cm) { return cm.setOption("mode", cm.getOption("mode")); });
        };
        return _this;
    }
    MLIRSyntaxHighlightPlugin.prototype.onload = function () {
        var _this = this;
        this.app.workspace.onLayoutReady(function () {
            _this.refreshLeaves();
        });
    };
    MLIRSyntaxHighlightPlugin.prototype.onunload = function () {
        // Delete all the codemirror modes, to disable the syntax highlighting
        // except the default ones, obviously
        for (var key in CodeMirror.modes) {
            if (CodeMirror.modes.hasOwnProperty(key) && !this.modesToKeep.includes(key)) {
                delete CodeMirror.modes[key];
            }
        }
        this.refreshLeaves();
    };
    return MLIRSyntaxHighlightPlugin;
}(obsidian.Plugin));

module.exports = MLIRSyntaxHighlightPlugin;


/* nosourcemap */