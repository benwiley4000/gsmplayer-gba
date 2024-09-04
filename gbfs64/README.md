#gbfs64

This is a copy of the gbfs program for creating gbfs archives.

The code has been modified so that filenames can take up to 64 characters, instead of 24.

It's also been modified to compile with CMake on Windows (using Qt Creator). The "basename" function from POSIX libgen has been replaced by a local re-implementation that works on Windows.

Modified by Ben Wiley 2024
