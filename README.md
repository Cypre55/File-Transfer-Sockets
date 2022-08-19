# File Transfer using Sockets

This project is a simplified implementation of File Transfer Protocol (FTP). It has a server (*ftpS.c*) and client (*ftpC.c*). Client provides a shell to perform various commands.

### Commands

| Command                            | Description                                                  |
| ---------------------------------- | ------------------------------------------------------------ |
| open *IP Port*                   | To open a TCP connection with server on given port           |
| user *username*                  | To enter username for authentication                         |
| pass *password*                  | To enter password for authentication                         |
| cd *dirname*                     | To change directories on server side                         |
| lcd *dirname*                    | To change directories on client side                         |
| dir                                | To show contents of current directory of server              |
| get *remote_file* *local_file* | To copy *remote_file* to client as *local_file*              |
| put *local_file* *remote_file* | To copy *local_file* to server as *remote_file*              |
| mget *file1* *file2* ...       | To copy multiple files to client (saved with same name) from server |
| mput *file1* *file2* ...       | To copy multiple files to server (saved with same name) from client |
| quit                               | To quit                                                      |

### Usage

```bash
# To run server
make server

# To run client
make client
```

### Credits

Built by **Satwik Chappidi** and **Nikhil Tudaha**
