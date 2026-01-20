<%*
let list = {
  "ℹ️ info" : "info,info",
  "✏️ note" : "note,note",
  "📒 summary" : "summary,summary",
  "🔥 tip" : "tip,tip",
  "☑️ check" : "check,check",
  "❔Help" : "help,help",
  "⚠️ Warning" : "warning,warning",
  "❌ Fail" : "fail,fail",
  "⚡Danger" : "danger,danger",
  "🪲 Bug" : "bug,bug",
  "📋 Example" : "example,example",
  "✍️ Quote " : "quote,quote",
  "😝 LOL " : "LOL,LOL",
  "📕 Reference " : "REF,Reference"
};
let keys = Object.keys(list);
key = await tp.system.suggester(keys, keys);
let value = list[key];
let index = value.indexOf(",");
let text = value.substring(index+1);
value = value.substring(0, index);
if (key) return ">[!" + value + "]+ " + text + "\n> ";
%>