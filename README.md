# Galcon
The old Digital Information Systems of Kentucky (D.I.S.K.) game of Galcon

Please note this source code is very old and almost definitely is filled with security holes and other bugs. I'm making this available for those who remember DISK and want a chance to see the old code and compile it on Linux.

The code has been refactored to compile with a few warnings on Ubuntu 14.04 but will likely work with any Gnu-Linux distro that has the gcc tools installed.

Please note that as it is, the generated galcon binary must be owned by root and have the setuid bit set.  This requirement could likely be resolved in the future by changing the permissions on the shared memory segment (IPCS).  Additionally, if memory serves, the game only supports a single active session.  Others can attach and watch the game as an Observer.

Also note that this game has many authors, reflected in the comments.  Not all have signed off on the release of the code, but given its age and lack of an original license, as the last known contributor to the code I'm declaring its license to be BSD.  I'd be happy to commit changes to the codebase in this repo if there is interest in bug fixes or revisions.

Please note that this is an independant work from the commercial Galcon game ( http://www.galcon.com/ ) and shares some common heritage, but no code. The author of the commercial Galcon game has made a nice history page here: http://www.galcon.com/classic/history.html

