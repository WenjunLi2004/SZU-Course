---
<%*
let title = tp.file.title;
if (title.startsWith("Untitled")) {
 title = await tp.system.prompt("Enter the title of the book");
 if(!title) return;
}
if (title == "") {
title = "Untitled";
} else {
await tp.file.rename(title);
}
await tp.file.move("/books/" + title)
-%>
type: books
title: "[[<% title %>]]"
cover: 
author: '[[<% tp.system.prompt("Enter the author of the book")%>]]'
genre: <% tp.system.suggester(["fiction", "non-fiction"], ["fiction", "non-fiction"], true, 'genre')%>
status: <% tp.system.suggester(["done", "reading", "to-read"], ["done", "reading", "to-read"], true, 'status')%>
rating: 
myRating: 
summary: 
date_creation: <% tp.file.creation_date("YYYY-MM-DD") %>
date_start: 
date_finish:
---
## 3 sentences to describe the book
1. <% tp.file.cursor() %>
2. 
3. 

## 5 take aways
1. 
2. 
3. 
4. 
5. 

## Review



## Related



## Reference




