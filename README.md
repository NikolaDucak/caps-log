# clog - Captain's Log 
`clog` is a small terminal based journaling tool. Daily entries are basic text files
with some small rules that allow better structuring of entries & easier parsing/lookup.. 
Logs are edited with your `$EDITOR` while clog is there for easier
navigation between those logs with calendar like ui & calendar overwiev of certain parts of entries

# Log entry syntax
`clog` stores all logs as simple md files purely to give your editor some syntax highlighting
when editing. Only syntax that is mandatory to provide sections & tasks features is
respecting the sections & tasks rules
## Section
`# section name`

## Tasks
`[<any signgle char>] (<optional time spent on task info>) task title : any other random task information`

# Building & installing
To build the `clog` executable run:
```
mkdir build && cd build && cmake ..
make 
```
After a successfull build. Run: `sudo make install`.

# todo
*esential*
[ ] check if editor env var is available
[ ] write a better README
[ ] rename everythin to camelCase
[ ] add ability to remove logs
[ ] add ability to add/remove tags/tasks

*non-esential*
[ ] shift + J/K skip whole month
[ ] opening a date with non existant log should create a log with some basic sections as skeleton
[ ] tests
[ ] preview window focuses parts of the text that matches
[ ] mouse wont focus unfocused menus
