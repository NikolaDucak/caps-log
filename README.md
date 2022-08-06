# clog - Captain's Log 
`clog` is a small terminal based journaling tool. Daily entries are represented as markdown-like files
with some small rules that allow better structuring of entries & easier parsing/lookup.
Currently clog uses your `$EDITOR` environment variable to start the editor. 

# Keybindings
Vim-like keybindings, 
    - hjkl or arrow = for navigating within calendar or menus
    - tab or shift + tab =  switch focus between calendar and menus
    - enter = highlight logs that contain tag/section or open log entry in `$EDITOR` for date under the cursor

# Log entry 'syntax'
`clog` stores all logs as simple markdown files purely to give your editor some syntax highlighting
when editing. Only syntax that is mandatory to provide sections & tags features is respecting the sections & tasks rules.
*Section*
`# section name`

## Tasks
TODO

# Building & installing
To build the `clog` executable run:
```
mkdir build && cd build && cmake ..
make 
```
After a successfull build. Run: `sudo make install`.

# Todo
*esential*
[ ] spike: how to use editor in browser
[ ] fix: check if editor env var is available
[ ] feat: write a better README
[ ] refactor: rename everythin to camelCase
[ ] feat: add ability to add/remove tags/tasks
[ ] feat: command line argument to pass in log directory
[ ] feat: dissplayin different years than current is not fully functional

*non-esential*
[ ] shift + J/K skip whole month
[ ] feat: opening a date with non existant log should create a log with some basic sections as skeleton
[ ] refactor: tests
[ ] feat: preview window focuses parts of the text that matches
[ ] feat: scrollable preview window
[ ] bug: mouse wont focus unfocused menus

