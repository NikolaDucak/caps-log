# Captain's Log - or `clog` for short
`clog` is a small terminal based journaling tool. Daily entries are saved as markdownfiles
with some small rules that allow better structuring of entries & easier parsing/lookup.
Currently clog uses your `$EDITOR` environment variable to start the editor. 

## Keybindings 
- `hjkl` or arrow = for navigating within calendar or menus
- `tab` or `shift + tab` =  switch focus between calendar and menus
- `enter` = highlight logs that contain tag/section or open log entry in `$EDITOR` for date under the cursor

## Log entry 'syntax'
`clog` stores all logs as simple markdown files purely to give your editor some syntax highlighting
when editing. Only syntax that is mandatory to provide sections & tags features is respecting the sections & tasks rules.

*Sections*
`# section name`

*Tags*
`* tag name`
`* tag name (some other optional info)`
`* tag name (some other optional info): a multiline body that is ignored by clog`
For examples of valid and invalid section and tag margers see [./test/log_entry_test.cpp](./test/log_entry_test.cpp)

# Building & installing
**Dependancies**
clog fetches most of it's dependancies from github except `boost`

To build the `clog` executable run:
```
mkdir build && cd build && cmake ..
make 
```
After a successfull build. Run: `sudo make install`.

# Planned work
- [ ] spike: editor in browser wasm build
- [ ] fix: check if editor env var is available
- [ ] refactor: write a better README
- [ ] feat: add ability to toggle tags from clog directly instead of opening the editor
- [ ] feat: command line argument to pass in log directory
- [ ] feat: dissplayin different years than current is not fully functional
- [ ] feat: local log repo wrapper that syncs files with github or mega.nz

lower priority work:
- [ ] shift + J/K skip whole month
- [ ] feat: opening a date with non existant log should create a log with some basic sections as skeleton
- [ ] feat: preview window focuses parts of the text that matches
- [ ] feat: scrollable preview window
- [ ] bug: mouse wont focus unfocused menus
