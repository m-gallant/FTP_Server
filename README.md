# FTP_Server

This project is a simple FTP server that was created as a school project and co-authored with https://github.com/julillae
Current commands supported are USER, CWD, CDUP, QUIT, MODE, RETR, PASV, NLST, TYPE and STRU.
USER must be anonymous, no password is required; once logged in, user will not be logged out until session ends, even if a different username is entered (as required by assignment description).


## Acknowledgements

The original dir.c, dir.h, usage.c, usage.h, and Makefile files were provided. dir.c was modified to include error checking, and the Makefile was modified to include the dataConnection class.
