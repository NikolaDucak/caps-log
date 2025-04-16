# Captain's Log - or `Caps-Log` for Short

`Caps-Log` is a small, terminal-based journaling tool.

![Caps-Log Screenshot](./caps-log.gif)

## What It Does

Daily entries are saved locally as Markdown files. Level 1 headers are
interpreted as 'sections', and unordered lists beginning with the '*' character
are interpreted as 'tags'. Titles of these sections and tags are then displayed
in two menus. When selecting a section, tag menu will list out only the tags 
found under the selected section. If no section is selected, tag menu will 
display all tags found. Selecting an item in these menus highlights the dates 
with mentions of that tag or title in the calendar. This feature provides a 
visual representation of how (in)consistent your habits and activities are.

Clicking on a date or pressing enter when a date is focused will open that log
file in an editor, if possible. Currently, `Caps-Log` uses your `$EDITOR`
environment variable to start the editor, if set. Otherwise, this functionality
is disabled. This integration looks particularly impressive with terminal-based
editors.

Additionally, `caps-log` also has a primitive 'remote storage' feture in the form
of using a git repository with a remote to push and pull data. (See
[Configuration & Command Line Options](#configuration--command-line-options)
section below)

**Note**

- Emoji handling is somewhat erratic, potentially due to an FTXUI issue or
  misuse on my part. This could also be an issue with my font or terminal
  emulator. When an emoji is drawn, it shifts all other characters to the right
  by one place.
- The tool is not optimal for use in small terminals. While still functional, it
  performs best when the entire calendar is visible.

# Getting It

It is recommended that you build and install it manually, see 
[Building and installing](#building--installing) section.

Or you can visit the [releases page](https://github.com/nikoladucak/caps-log/releases)
and download a `caps-log-<platform>.tar.gz` file that contains a prebuilt binary, but 
for now only x86 Linux and intel MacOS platforms are supported. This will be 
improved in the future.

## Keybindings

- `Tab` or `Shift + Tab` = Switch focus between the calendar and menus.
- `h` or arrow keys = Navigate within the calendar or menus.
- `Enter` = Highlight logs containing a specific tag/section or open the log
  entry in `$EDITOR` for the date under the cursor.
- `D` = Delete the log under the cursor if the calendar is focused.
- `+` / `-` = Navigate to the next / previous year's calendar.

## Log Entry Tags and Sections

`Caps-Log` stores all logs as simple Markdown files to enable syntax
highlighting in your editor. This 'syntax' was chosen based on personal
preference. If you find flaws, please open an issue. It's up to you to decide
what constitutes a section and what a tag.

_Sections_

To mark a section, use: `# Section Name`. All text below until the next section
is considered part of it. By default, the first line of the file
is ignored when identifying section titles, as it's commonly used for the date
of the entry. This behavior can be configured via command line arguments or a
config file.

_Tags_

Tags can be added to the log entry by starting a line with `* Tag Name`. Tags
can belong to a section, but they don't have to. If they don't belong to a
section, they are considered to belong under a `<root section>` which is 
displayed in the section menu.

- `* Tag Name`
- `* Tag Name (Additional Information)`
- `* Tag Name (Additional Information): A multiline body that is ignored by Caps-Log`
  For examples of valid and invalid sections and tags, see
  [./test/log_entry_test.cpp](./test/log_entry_test.cpp)

## Encrypting Logs

`Caps-Log` can encrypt your logs using the AES encryption algorithm. 
If you open an encrypted log repo, you will be prompted to enter a password. 
In case you enter a wrong password, caps-log will notify you end exit.
Same if you provide a password for a non encrypted log repository.

```
# Note: Caps-Log will ignore files not matching the log filename format.
# You can also provide --log-dir-path and --log-filename-format to control behavior.
# Encrypt
caps-log --encrypt --password <your password>
# Decrypt
caps-log --decrypt --password <your password>
```

## Configuration & Command Line Options

_Command Line Options_

```
Allowed options:
  -h [ --help ]                         Show this message.
  -c [ --config ] arg                   Override the default config file path
                                        (~/.caps-log/config.ini).
  --log-dir-path arg (=~/.caps-log/day/)
                                        Path where log files are stored.
  --log-name-format arg (=d%y_%m_%d.md) Format in which log entry markdown
                                        files are saved.
  --sunday-start                        Display Sunday as the first day of the
                                        week in the calendar.
  --first-line-section                  If a section mark is placed on the
                                        first line, override the default
                                        behavior of ignoring it.
  --password arg                        Password for encrypted log repositories
                                        or to be used with --encrypt/--decrypt.
  --encrypt                             Apply encryption to all logs in the log
                                        directory path (requires --password).
  --decrypt                             Apply decryption to all logs in the log
                                        directory path (requires --password).
```

_Config File_

most of the command line flags can be set through a config file.

```ini
log-dir-path=/path/to/log/dir
log-name-format=%y_%d_%m.txt
sunday-start=true
first-line-section=true
password=your-password
```

Config file also allows configuring caps-log to treat the directory where logs
are stored as a git repository with a remote set-up. Upon exiting, `caps-log`
will commit and push all changes to the remote. Note that currently, only the
remotes with ssh authentication are supported. Here is an example of a config
file for git remote log repository.

```ini
# NOTE: this must be a path to a directory that is 
# inside the git.repo-root, otherwise `caps-log` will fail to start.
log-dir-path=/Users/me/.caps-log/clog-entries/days

[git]
# setting it to false, or not setting it at all is the 
# same as not having the below options defined
enable-git-log-repo=true 
ssh-key-path=/Users/me/.ssh/id_rsa # required
ssh-pub-key-path=/Users/me/.ssh/id_rsa.pub # required
repo-root=/Users/me/.caps-log/clog-entries/
remote-name=something # 'origin' is the default
main-branch-name=main # 'master' is the default

```

__Annual events__
Caps-log can also be configured to display annual events in the calendar. They 
will be highlighted with green color and number of days since or until the event (if the 
total number of days is less that `recent-events-window` days) will be displayed in the
small box to the left of the calendar.
```ini
[calendar-events]
recent-events-window=60

[calendar-events.birthadays.0]
name=Joe
date=18.07
[calendar-events.birthadays.1]
name=Jane
date=03.07.
[calendar-events.holidays.0]
name=New Years eve
date=01.01.
[calendar-events.holidays.1]
name=New Year
date=31.12.
```

## Building & Installing

**Dependencies**

`Caps-Log` fetches its dependencies from GitHub, except for Boost and `libgit2`.
You should have Boost program options installed. This typically involves:

```shell
# Linux
sudo apt-get install -y libboost-program-options-dev libgit2-dev
# Mac
brew install boost libgit2
```

To build the `Caps-Log` executable, run:

```shell
mkdir build && cd build && cmake ..
make
```

After a successful build, to install the executable in a common `$PATH`
location, run: `sudo make install`. Then, you can simply start the application
by typing `caps-log` in your terminal.

If you wish to build and run the tests, execute:

```shell
mkdir build && cd build && cmake .. -DCAPS_LOG_BUILD_TESTS=ON
make 
ctest
```
