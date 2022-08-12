# Captain's Log - or `clog` for short
`clog` is a small terminal based journaling tool. 

It saves daily entries as mardown files in a local directory, it uses bullet points and highest level headings 
as 'tags' and 'sections' that are parsed from files and displayed on menues. Selecting one of the items in menues
will highlight the dates in caledar where that section or tag. It's can be used to keep track of how incosistent you are.

Clicking on a date or pressing enter when a date is focused will (maybe) open that log file in an editor.
Currently clog uses your `$EDITOR` environment variable to start the editor, if that environment variable is set. Otherwise
this functionality is disabled.

## Keybindings 
- `hjkl` or arrow keys = for navigating within calendar or menus
- `tab` or `shift + tab` =  switch focus between calendar and menus
- `enter` = highlight logs that contain tag/section or open log entry in `$EDITOR` for date under the cursor

## Log entry tags and sections
`clog` stores all logs as simple markdown files purely to give your editor some syntax highlighting
when editing. This 'syntax' was chosen with little thought based on personal preference. If you find flaws, do open an issue.

*Sections*
To mark a section just do: `# section name`. All text below untill the next section is considered part of it, although 
clog currently does not care for that text, just the section title. By default, the first line of the file is ignored when
looking up section titles, as I like to use that line for a section with the date of the current line, this will be configurable
in the future.

*Tags*
The tags do not apply to a specific section but to the whole log entry file. 
- `* tag name`
- `* tag name (some other optional info)`
- `* tag name (some other optional info): a multiline body that is ignored by clog`
For examples of valid and invalid section and tag margers see [./test/log_entry_test.cpp](./test/log_entry_test.cpp)

# Configuration & command line options
Currently clog silently ignores unknown options or bad arguments

*Command line options*
```
clog (Captains Log)
A small TUI journaling tool.

 -h --help                     - show this message
 -c --config <path>            - override the default config file path (~/.clog/config.ini)
 --log-dir-path <path>         - path where log files are stored (default: ~/.clog/day/)
 --log-name-format <format>    - format in which log entry markdown files are saved (default: d%Y_%m_%d.md)
 --sunday-start                - have the calendar display sunday as first day of the week)"};
```

*Config file*
```
log-dir-path = /path/to/log/dir
log-name-format = %Y_%d_%m.txt
sunday-start = true
```




# Building & installing
**Dependancies**

`clog` fetches it's dependancies from github, so no extra action should be necessary (I hope).

To build the `clog` executable, run:
```sh
mkdir build && cd build && cmake ..
make 
```
After a successfull build, to install the executable where it most likely will be visible with your current `$PATH` Run: `sudo make install`.
Then you can just run the app by typing `clog` in your terminal.

If you want to build and run the tests do:
```sh
mkdir build && cd build && cmake .. -DBUILD_TESTS=ON
make 
ctest
```

# Planned work
- [ ] test: add date tests
- [ ] feat: command line arguments
    - [ ] log directory path 
    - [ ] log filename format
    - [ ] sunday first
    - [ ] enable old task syntax as tag
    - [ ] ignore first line section
- [ ] feat: cofiguration file

- [ ] refactor: write a better README
- [ ] feat: add ability to toggle tags from clog directly instead of opening the editor
- [ ] feat: displaying different years than current is not fully functional
- [ ] ? feat: local log repo wrapper that syncs files with github or mega.nz
- [ ] ? feat: repeat events: forgetable things like birthdays and aniversaries are highlighted if user wants to do so.

lower priority work:
- [ ] feat: color weekend dates in callendar a different collor
- [ ] feat: shift + J/K skip whole month
- [ ] feat: opening a date with non existant log should create a log with some basic sections as skeleton
- [ ] feat: preview window focuses parts of the text that matches
- [ ] feat: scrollable preview window
- [ ] fix: mouse wont focus unfocused menus
