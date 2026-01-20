---
type: books
title: "[[<% tp.file.title %>]]"
cover: 
author:
  - '[[<% tp.system.prompt("请输入作者")%>]]'
genre: <% tp.system.suggester(["fiction", "non-fiction"], ["fiction", "non-fiction"], true, 'genre')%>
status: <% tp.system.suggester(["to-read", "reading", "done"], ["to-read", "reading", "done"], true, 'status')%>
rating: 
myRating: 
summary: 
date_creation: <% tp.file.creation_date("YYYY-MM-DD") %>
date_start: 
date_finish:
---
## 3 sentences to describe the book
1. 
2. 
3. 

## 5 take aways
1. 
2. 
3. 
4. 
5. 

## Review
<% tp.file.cursor() %>


## Related



## Reference
