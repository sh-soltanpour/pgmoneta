/*
 * Copyright (C) 2024 The pgmoneta community
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* pgmoneta */
#include <pgmoneta.h>
#include <configuration.h>
#include <json.h>
#include <logging.h>
#include <management.h>
#include <network.h>
#include <security.h>
#include <shmem.h>
#include <utils.h>
#include <value.h>
#include <verify.h>

/* system */
#include <err.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <openssl/ssl.h>

#define HELP 99

#define COMMAND_BACKUP "backup"
#define COMMAND_LIST_BACKUP "list-backup"
#define COMMAND_RESTORE "restore"
#define COMMAND_VERIFY "verify"
#define COMMAND_ARCHIVE "archive"
#define COMMAND_DELETE "delete"
#define COMMAND_RETAIN "retain"
#define COMMAND_EXPUNGE "expunge"
#define COMMAND_ENCRYPT "encrypt"
#define COMMAND_DECRYPT "decrypt"
#define COMMAND_COMPRESS "compress"
#define COMMAND_DECOMPRESS "decompress"
#define COMMAND_PING "ping"
#define COMMAND_STOP "stop"
#define COMMAND_STATUS "status"
#define COMMAND_CONF "conf"
#define COMMAND_CLEAR "clear"
#define COMMAND_INFO "info"
#define COMMAND_ANNOTATE "annotate"

static void help_backup(void);
static void help_list_backup(void);
static void help_restore(void);
static void help_verify(void);
static void help_archive(void);
static void help_delete(void);
static void help_retain(void);
static void help_expunge(void);
static void help_decrypt(void);
static void help_encrypt(void);
static void help_decompress(void);
static void help_compress(void);
static void help_stop(void);
static void help_ping(void);
static void help_status_details(void);
static void help_conf(void);
static void help_clear(void);
static void help_info(void);
static void help_annotate(void);
static void display_helper(char* command);

static int backup(SSL* ssl, int socket, char* server, int32_t output_format);
static int list_backup(SSL* ssl, int socket, char* server, int32_t output_format);
static int restore(SSL* ssl, int socket, char* server, char* backup_id, char* position, char* directory, int32_t output_format);
static int verify(SSL* ssl, int socket, char* server, char* backup_id, char* directory, char* files, int32_t output_format);
static int archive(SSL* ssl, int socket, char* server, char* backup_id, char* position, char* directory, int32_t output_format);
static int delete(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format);
static int stop(SSL* ssl, int socket, int32_t output_format);
static int status(SSL* ssl, int socket, int32_t output_format);
static int details(SSL* ssl, int socket, int32_t output_format);
static int ping(SSL* ssl, int socket, int32_t output_format);
static int reset(SSL* ssl, int socket, int32_t output_format);
static int reload(SSL* ssl, int socket, int32_t output_format);
static int retain(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format);
static int expunge(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format);
static int decrypt_data(SSL* ssl, int socket, char* path, int32_t output_format);
static int encrypt_data(SSL* ssl, int socket, char* path, int32_t output_format);
static int decompress_data(SSL* ssl, int socket, char* path, int32_t output_format);
static int compress_data(SSL* ssl, int socket, char* path, int32_t output_format);
static int info(SSL* ssl, int socket, char* server, char* backup, int32_t output_format);
static int annotate(SSL* ssl, int socket, char* server, char* backup, char* command, char* key, char* comment, int32_t output_format);

static int  process_result(SSL* ssl, int socket, int32_t output_format);

static void
version(void)
{
   printf("pgmoneta-cli %s\n", VERSION);
   exit(1);
}

static void
usage(void)
{
   printf("pgmoneta-cli %s\n", VERSION);
   printf("  Command line utility for pgmoneta\n");
   printf("\n");

   printf("Usage:\n");
   printf("  pgmoneta-cli [ -c CONFIG_FILE ] [ COMMAND ] \n");
   printf("\n");
   printf("Options:\n");
   printf("  -c, --config CONFIG_FILE Set the path to the pgmoneta.conf file\n");
   printf("  -h, --host HOST          Set the host name\n");
   printf("  -p, --port PORT          Set the port number\n");
   printf("  -U, --user USERNAME      Set the user name\n");
   printf("  -P, --password PASSWORD  Set the password\n");
   printf("  -L, --logfile FILE       Set the log file\n");
   printf("  -v, --verbose            Output text string of result\n");
   printf("  -V, --version            Display version information\n");
   printf("  -F, --format text|json   Set the output format\n");
   printf("  -?, --help               Display help\n");
   printf("\n");
   printf("Commands:\n");
   printf("  backup                   Backup a server\n");
   printf("  list-backup              List the backups for a server\n");
   printf("  restore                  Restore a backup from a server\n");
   printf("  verify                   Verify a backup from a server\n");
   printf("  archive                  Archive a backup from a server\n");
   printf("  delete                   Delete a backup from a server\n");
   printf("  retain                   Retain a backup from a server\n");
   printf("  expunge                  Expunge a backup from a server\n");
   printf("  encrypt                  Encrypt a file using master-key\n");
   printf("  decrypt                  Decrypt a file using master-key\n");
   printf("  compress                 Compress a file using configured method\n");
   printf("  decompress               Decompress a file using configured method\n");
   printf("  info                     Information about a backup\n");
   printf("  annotate                 Annotate a backup with comments\n");
   printf("  ping                     Check if pgmoneta is alive\n");
   printf("  stop                     Stop pgmoneta\n");
   printf("  status [details]         Status of pgmoneta, with optional details\n");
   printf("  conf <action>            Manage the configuration, with one of subcommands:\n");
   printf("                           - 'reload' to reload the configuration\n");
   printf("  clear <what>             Clear data, with:\n");
   printf("                           - 'prometheus' to reset the Prometheus statistics\n");
   printf("\n");
   printf("pgmoneta: %s\n", PGMONETA_HOMEPAGE);
   printf("Report bugs: %s\n", PGMONETA_ISSUES);
}

const struct pgmoneta_command command_table[] = {
   {
      .command = "backup",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_BACKUP,
      .deprecated = false,
      .log_message = "<backup> [%s]",
   },
   {
      .command = "list-backup",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_LIST_BACKUP,
      .deprecated = false,
      .log_message = "<list-backup> [%s]",
   },
   {
      .command = "restore",
      .subcommand = "",
      .accepted_argument_count = {3, 4},
      .action = MANAGEMENT_RESTORE,
      .deprecated = false,
      .log_message = "<restore> [%s]",
   },
   {
      .command = "verify",
      .subcommand = "",
      .accepted_argument_count = {3, 4},
      .action = MANAGEMENT_VERIFY,
      .deprecated = false,
      .log_message = "<verify> [%s]",
   },
   {
      .command = "archive",
      .subcommand = "",
      .accepted_argument_count = {3, 4},
      .action = MANAGEMENT_ARCHIVE,
      .deprecated = false,
      .log_message = "<archive> [%s]"
   },
   {
      .command = "delete",
      .subcommand = "",
      .accepted_argument_count = {2},
      .action = MANAGEMENT_DELETE,
      .deprecated = false,
      .log_message = "<delete> [%s]"
   },
   {
      .command = "retain",
      .subcommand = "",
      .accepted_argument_count = {2},
      .action = MANAGEMENT_RETAIN,
      .deprecated = false,
      .log_message = "<retain> [%s]"
   },
   {
      .command = "expunge",
      .subcommand = "",
      .accepted_argument_count = {2},
      .action = MANAGEMENT_EXPUNGE,
      .deprecated = false,
      .log_message = "<expunge [%s]>",
   },
   {
      .command = "decrypt",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_DECRYPT,
      .deprecated = false,
      .log_message = "<decrypt> [%s]"
   },
   {
      .command = "encrypt",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_ENCRYPT,
      .deprecated = false,
      .log_message = "<encrypt> [%s]"
   },
   {
      .command = "decompress",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_DECOMPRESS,
      .deprecated = false,
      .log_message = "<decompress> [%s]"
   },
   {
      .command = "compress",
      .subcommand = "",
      .accepted_argument_count = {1},
      .action = MANAGEMENT_COMPRESS,
      .deprecated = false,
      .log_message = "<compress> [%s]"
   },
   {
      .command = "ping",
      .subcommand = "",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_PING,
      .deprecated = false,
      .log_message = "<ping>"
   },
   {
      .command = "stop",
      .subcommand = "",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_STOP,
      .deprecated = false,
      .log_message = "<stop>"
   },
   {
      .command = "status",
      .subcommand = "",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_STATUS,
      .deprecated = false,
      .log_message = "<status>"
   },
   {
      .command = "status",
      .subcommand = "details",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_STATUS_DETAILS,
      .deprecated = false,
      .log_message = "<status details>"
   },
   {
      .command = "conf",
      .subcommand = "reload",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_RELOAD,
      .deprecated = false,
      .log_message = "<conf reload>"
   },
   {
      .command = "clear",
      .subcommand = "prometheus",
      .accepted_argument_count = {0},
      .action = MANAGEMENT_RESET,
      .deprecated = false,
      .log_message = "<clear prometheus>"
   },
   {
      .command = "info",
      .subcommand = "",
      .accepted_argument_count = {2},
      .action = MANAGEMENT_INFO,
      .deprecated = false,
      .log_message = "<info> [%s]"
   },
   {
      .command = "annotate",
      .subcommand = "",
      .accepted_argument_count = {4, 5},
      .action = MANAGEMENT_ANNOTATE,
      .deprecated = false,
      .log_message = "<annotate> [%s]"
   }
};

int
main(int argc, char** argv)
{
   int socket = -1;
   SSL* s_ssl = NULL;
   int ret;
   int exit_code = 0;
   char* configuration_path = NULL;
   char* host = NULL;
   char* port = NULL;
   char* username = NULL;
   char* password = NULL;
   bool verbose = false;
   char* logfile = NULL;
   bool do_free = true;
   int c;
   int option_index = 0;
   /* Store the result from command parser*/
   bool matched = false;
   size_t size;
   char un[MAX_USERNAME_LENGTH];
   struct configuration* config = NULL;
   int32_t output_format = MANAGEMENT_OUTPUT_FORMAT_TEXT;
   size_t command_count = sizeof(command_table) / sizeof(struct pgmoneta_command);
   struct pgmoneta_parsed_command parsed = {.cmd = NULL, .args = {0}};

   // Disable stdout buffering (i.e. write to stdout immediatelly).
   setbuf(stdout, NULL);

   while (1)
   {
      static struct option long_options[] =
      {
         {"config", required_argument, 0, 'c'},
         {"host", required_argument, 0, 'h'},
         {"port", required_argument, 0, 'p'},
         {"user", required_argument, 0, 'U'},
         {"password", required_argument, 0, 'P'},
         {"logfile", required_argument, 0, 'L'},
         {"verbose", no_argument, 0, 'v'},
         {"version", no_argument, 0, 'V'},
         {"format", required_argument, 0, 'F'},
         {"help", no_argument, 0, '?'}
      };

      c = getopt_long(argc, argv, "vV?c:h:p:U:P:L:F:",
                      long_options, &option_index);

      if (c == -1)
      {
         break;
      }

      switch (c)
      {
         case 'c':
            configuration_path = optarg;
            break;
         case 'h':
            host = optarg;
            break;
         case 'p':
            port = optarg;
            break;
         case 'U':
            username = optarg;
            break;
         case 'P':
            password = optarg;
            break;
         case 'L':
            logfile = optarg;
            break;
         case 'v':
            verbose = true;
            break;
         case 'V':
            version();
            break;
         case 'F':
            if (!strncmp(optarg, "json", MISC_LENGTH))
            {
               output_format = MANAGEMENT_OUTPUT_FORMAT_JSON;
            }
            else
            {
               output_format = MANAGEMENT_OUTPUT_FORMAT_TEXT;
            }
            break;
         case '?':
            usage();
            exit(1);
            break;
         default:
            break;
      }
   }

   if (getuid() == 0)
   {
      warnx("pgmoneta-cli: Using the root account is not allowed");
      exit(1);
   }

   if (configuration_path != NULL && (host != NULL || port != NULL))
   {
      warnx("pgmoneta-cli: Use either -c or -h/-p to define endpoint");
      exit(1);
   }

   if (argc <= 1)
   {
      usage();
      exit(1);
   }

   size = sizeof(struct configuration);
   if (pgmoneta_create_shared_memory(size, HUGEPAGE_OFF, &shmem))
   {
      warnx("pgmoneta-cli: Error creating shared memory");
      exit(1);
   }
   pgmoneta_init_configuration(shmem);

   if (configuration_path != NULL)
   {
      ret = pgmoneta_read_configuration(shmem, configuration_path);
      if (ret)
      {
         warnx("pgmoneta-cli: Configuration not found: %s", configuration_path);
         exit(1);
      }

      if (logfile)
      {
         config = (struct configuration*)shmem;

         config->log_type = PGMONETA_LOGGING_TYPE_FILE;
         memset(&config->log_path[0], 0, MISC_LENGTH);
         memcpy(&config->log_path[0], logfile, MIN(MISC_LENGTH - 1, strlen(logfile)));
      }

      if (pgmoneta_start_logging())
      {
         exit(1);
      }

      config = (struct configuration*)shmem;
   }
   else
   {
      ret = pgmoneta_read_configuration(shmem, "/etc/pgmoneta/pgmoneta.conf");
      if (ret)
      {
         if (host == NULL || port == NULL)
         {
            warnx("pgmoneta-cli: Host and port must be specified");
            exit(1);
         }
      }
      else
      {
         configuration_path = "/etc/pgmoneta/pgmoneta.conf";

         if (logfile)
         {
            config = (struct configuration*)shmem;

            config->log_type = PGMONETA_LOGGING_TYPE_FILE;
            memset(&config->log_path[0], 0, MISC_LENGTH);
            memcpy(&config->log_path[0], logfile, MIN(MISC_LENGTH - 1, strlen(logfile)));
         }

         if (pgmoneta_start_logging())
         {
            exit(1);
         }

         config = (struct configuration*)shmem;
      }
   }
   if (!parse_command(argc, argv, optind, &parsed, command_table, command_count))
   {
      if (argc > optind)
      {
         char* command = argv[optind];
         display_helper(command);
      }
      else
      {
         usage();
      }
      exit_code = 1;
      goto done;
   }

   matched = true;

   if (configuration_path != NULL)
   {
      /* Local connection */
      if (pgmoneta_connect_unix_socket(config->unix_socket_dir, MAIN_UDS, &socket))
      {
         exit_code = 1;
         goto done;
      }
   }
   else
   {
      /* Remote connection */
      if (pgmoneta_connect(host, atoi(port), &socket))
      {
         warnx("pgmoneta-cli: No route to host: %s:%s", host, port);
         goto done;
      }

      /* User name */
      if (username == NULL)
      {
username:
         printf("User name: ");

         memset(&un, 0, sizeof(un));
         if (fgets(&un[0], sizeof(un), stdin) == NULL)
         {
            exit_code = 1;
            goto done;
         }
         un[strlen(un) - 1] = 0;
         username = &un[0];
      }

      if (username == NULL || strlen(username) == 0)
      {
         goto username;
      }

      /* Password */
      if (password == NULL)
      {
password:
         if (password != NULL)
         {
            free(password);
            password = NULL;
         }

         printf("Password : ");
         password = pgmoneta_get_password();
         printf("\n");
      }
      else
      {
         do_free = false;
      }

      for (size_t i = 0; i < strlen(password); i++)
      {
         if ((unsigned char)(*(password + i)) & 0x80)
         {
            goto password;
         }
      }

      /* Authenticate */
      if (pgmoneta_remote_management_scram_sha256(username, password, socket, &s_ssl) != AUTH_SUCCESS)
      {
         warnx("pgmoneta-cli: Bad credentials for %s", username);
         goto done;
      }
   }

   if (parsed.cmd->action == MANAGEMENT_BACKUP)
   {
      exit_code = backup(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_LIST_BACKUP)
   {
      exit_code = list_backup(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_RESTORE)
   {
      if (parsed.args[3])
      {
         exit_code = restore(s_ssl, socket, parsed.args[0], parsed.args[1], parsed.args[2], parsed.args[3], output_format);
      }
      else
      {
         exit_code = restore(s_ssl, socket, parsed.args[0], parsed.args[1], NULL, parsed.args[2], output_format);
      }
   }
   else if (parsed.cmd->action == MANAGEMENT_VERIFY)
   {
      if (parsed.args[3])
      {
         exit_code = verify(s_ssl, socket, parsed.args[0], parsed.args[1], parsed.args[2], parsed.args[3], output_format);
      }
      else
      {
         exit_code = verify(s_ssl, socket, parsed.args[0], parsed.args[1], parsed.args[2], "failed", output_format);
      }
   }
   else if (parsed.cmd->action == MANAGEMENT_ARCHIVE)
   {
      if (parsed.args[3])
      {
         exit_code = archive(s_ssl, socket, parsed.args[0], parsed.args[1], parsed.args[2], parsed.args[3], output_format);
      }
      else
      {
         exit_code = archive(s_ssl, socket, parsed.args[0], parsed.args[1], NULL, parsed.args[2], output_format);
      }
   }
   else if (parsed.cmd->action == MANAGEMENT_DELETE)
   {
      exit_code = delete(s_ssl, socket, parsed.args[0], parsed.args[1], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_STOP)
   {
      exit_code = stop(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_STATUS)
   {
      exit_code = status(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_STATUS_DETAILS)
   {
      exit_code = details(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_PING)
   {
      exit_code = ping(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_RESET)
   {
      exit_code = reset(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_RELOAD)
   {
      exit_code = reload(s_ssl, socket, output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_RETAIN)
   {
      exit_code = retain(s_ssl, socket, parsed.args[0], parsed.args[1], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_EXPUNGE)
   {
      exit_code = expunge(s_ssl, socket, parsed.args[0], parsed.args[1], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_DECRYPT)
   {
      exit_code = decrypt_data(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_ENCRYPT)
   {
      exit_code = encrypt_data(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_DECOMPRESS)
   {
      exit_code = decompress_data(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_COMPRESS)
   {
      exit_code = compress_data(s_ssl, socket, parsed.args[0], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_INFO)
   {
      exit_code = info(s_ssl, socket, parsed.args[0], parsed.args[1], output_format);
   }
   else if (parsed.cmd->action == MANAGEMENT_ANNOTATE)
   {
      exit_code = annotate(s_ssl, socket, parsed.args[0], parsed.args[1], parsed.args[2], parsed.args[3], parsed.args[4], output_format);
   }

done:

   if (s_ssl != NULL)
   {
      int res;
      SSL_CTX* ctx = SSL_get_SSL_CTX(s_ssl);
      res = SSL_shutdown(s_ssl);
      if (res == 0)
      {
         SSL_shutdown(s_ssl);
      }
      SSL_free(s_ssl);
      SSL_CTX_free(ctx);
   }

   pgmoneta_disconnect(socket);

   if (configuration_path != NULL)
   {
      if (matched && exit_code != 0)
      {
         warnx("No connection to pgmoneta on %s", config->unix_socket_dir);
      }
   }

   pgmoneta_stop_logging();
   pgmoneta_destroy_shared_memory(shmem, size);

   if (do_free)
   {
      free(password);
   }

   if (verbose)
   {
      if (exit_code == 0)
      {
         printf("Success (0)\n");
      }
      else
      {
         printf("Error (%d)\n", exit_code);
      }
   }

   return exit_code;
}

static void
help_backup(void)
{
   printf("Backup a server\n");
   printf("  pgmoneta-cli backup <server>\n");
}

static void
help_list_backup(void)
{
   printf("List backups for a server\n");
   printf("  pgmoneta-cli list-backup <server>\n");
}

static void
help_restore(void)
{
   printf("Restore a backup for a server\n");
   printf("  pgmoneta-cli restore <server> <timestamp|oldest|newest> [[current|name=X|xid=X|lsn=X|time=X|inclusive=X|timeline=X|action=X|primary|replica],*] <directory>\n");
}

static void
help_verify(void)
{
   printf("Verify a backup for a server\n");
   printf("  pgmoneta-cli verify <server> <timestamp|oldest|newest> <directory> [failed|all]\n");
}

static void
help_archive(void)
{
   printf("Archive a backup for a server\n");
   printf("  pgmoneta-cli archive <server> <timestamp|oldest|newest> [[current|name=X|xid=X|lsn=X|time=X|inclusive=X|timeline=X|action=X|primary|replica],*] <directory>\n");
}

static void
help_delete(void)
{
   printf("Delete a backup for a server\n");
   printf("  pgmoneta-cli delete <server> <timestamp|oldest|newest>\n");
}

static void
help_retain(void)
{
   printf("Retain a backup for a server\n");
   printf("  pgmoneta-cli retain <server> <timestamp|oldest|newest>\n");
}

static void
help_expunge(void)
{
   printf("Expunge a backup for a server\n");
   printf("  pgmoneta-cli expunge <server> <timestamp|oldest|newest>\n");
}

static void
help_decrypt(void)
{
   printf("Decrypt an .aes file created by pgmoneta-cli archive\n");
   printf("  pgmoneta-cli decrypt <file>\n");
}

static void
help_encrypt(void)
{
   printf("Encrypt a single file in place\n");
   printf("  pgmoneta-cli encrypt <file>\n");
}

static void
help_decompress(void)
{
   printf("Decompress a file using configured method\n");
   printf("  pgmoneta-cli decompress <file>\n");
}

static void
help_compress(void)
{
   printf("Compress a single file using configured method\n");
   printf("  pgmoneta-cli compress <file>\n");
}

static void
help_stop(void)
{
   printf("Stop pgmoneta\n");
   printf("  pgmoneta-cli stop\n");
}

static void
help_ping(void)
{
   printf("Check if pgmoneta is alive\n");
   printf("  pgmoneta-cli ping\n");
}

static void
help_status_details(void)
{
   printf("Status of pgmoneta\n");
   printf("  pgmoneta-cli status [details]\n");
}

static void
help_conf(void)
{
   printf("Manage the configuration\n");
   printf("  pgmoneta-cli conf [reload]\n");
}

static void
help_clear(void)
{
   printf("Reset data\n");
   printf("  pgmoneta-cli clear [prometheus]\n");
}

static void
help_info(void)
{
   printf("Information about a backup\n");
   printf("  pgmoneta-cli info <server> <backup>\n");
}

static void
help_annotate(void)
{
   printf("Annotate a backup with comments\n");
   printf("  pgmoneta-cli annotate <server> <timestamp|oldest|newest> <add|update|remove> <key> [comment]\n");
}

static void
display_helper(char* command)
{
   if (!strcmp(command, COMMAND_BACKUP))
   {
      help_backup();
   }
   else if (!strcmp(command, COMMAND_LIST_BACKUP))
   {
      help_list_backup();
   }
   else if (!strcmp(command, COMMAND_RESTORE))
   {
      help_restore();
   }
   else if (!strcmp(command, COMMAND_VERIFY))
   {
      help_verify();
   }
   else if (!strcmp(command, COMMAND_ARCHIVE))
   {
      help_archive();
   }
   else if (!strcmp(command, COMMAND_DELETE))
   {
      help_delete();
   }
   else if (!strcmp(command, COMMAND_RETAIN))
   {
      help_retain();
   }
   else if (!strcmp(command, COMMAND_EXPUNGE))
   {
      help_expunge();
   }
   else if (!strcmp(command, COMMAND_DECRYPT))
   {
      help_decrypt();
   }
   else if (!strcmp(command, COMMAND_ENCRYPT))
   {
      help_encrypt();
   }
   else if (!strcmp(command, COMMAND_DECOMPRESS))
   {
      help_decompress();
   }
   else if (!strcmp(command, COMMAND_COMPRESS))
   {
      help_compress();
   }
   else if (!strcmp(command, COMMAND_PING))
   {
      help_ping();
   }
   else if (!strcmp(command, COMMAND_STOP))
   {
      help_stop();
   }
   else if (!strcmp(command, COMMAND_STATUS))
   {
      help_status_details();
   }
   else if (!strcmp(command, COMMAND_CONF))
   {
      help_conf();
   }
   else if (!strcmp(command, COMMAND_CLEAR))
   {
      help_clear();
   }
   else if (!strcmp(command, COMMAND_INFO))
   {
      help_info();
   }
   else if (!strcmp(command, COMMAND_ANNOTATE))
   {
      help_annotate();
   }
   else
   {
      usage();
   }
}

static int
backup(SSL* ssl, int socket, char* server, int32_t output_format)
{
   if (pgmoneta_management_request_backup(ssl, socket, server, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
list_backup(SSL* ssl, int socket, char* server, int32_t output_format)
{
   if (pgmoneta_management_request_list_backup(ssl, socket, server, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
restore(SSL* ssl, int socket, char* server, char* backup_id, char* position, char* directory, int32_t output_format)
{
   if (pgmoneta_management_request_restore(ssl, socket, server, backup_id, position, directory, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
verify(SSL* ssl, int socket, char* server, char* backup_id, char* directory, char* files, int32_t output_format)
{
   if (pgmoneta_management_request_verify(ssl, socket, server, backup_id, directory, files, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
archive(SSL* ssl, int socket, char* server, char* backup_id, char* position, char* directory, int32_t output_format)
{
   if (pgmoneta_management_request_archive(ssl, socket, server, backup_id, position, directory, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
delete(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format)
{
   if (pgmoneta_management_request_delete(ssl, socket, server, backup_id, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
stop(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_stop(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
status(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_status(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
details(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_status_details(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
ping(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_ping(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
reset(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_reset(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
reload(SSL* ssl, int socket, int32_t output_format)
{
   if (pgmoneta_management_request_reload(ssl, socket, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
retain(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format)
{
   if (pgmoneta_management_request_retain(ssl, socket, server, backup_id, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
expunge(SSL* ssl, int socket, char* server, char* backup_id, int32_t output_format)
{
   if (pgmoneta_management_request_expunge(ssl, socket, server, backup_id, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
decrypt_data(SSL* ssl, int socket, char* path, int32_t output_format)
{
   if (pgmoneta_management_request_decrypt(ssl, socket, path, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
encrypt_data(SSL* ssl, int socket, char* path, int32_t output_format)
{
   if (pgmoneta_management_request_encrypt(ssl, socket, path, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
decompress_data(SSL* ssl, int socket, char* path, int32_t output_format)
{
   if (pgmoneta_management_request_decompress(ssl, socket, path, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
compress_data(SSL* ssl, int socket, char* path, int32_t output_format)
{
   if (pgmoneta_management_request_compress(ssl, socket, path, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
info(SSL* ssl, int socket, char* server, char* backup, int32_t output_format)
{
   if (pgmoneta_management_request_info(ssl, socket, server, backup, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
annotate(SSL* ssl, int socket, char* server, char* backup, char* action, char* key, char* comment, int32_t output_format)
{
   if (pgmoneta_management_request_annotate(ssl, socket, server, backup, action, key, comment, output_format))
   {
      goto error;
   }

   if (process_result(ssl, socket, output_format))
   {
      goto error;
   }

   return 0;

error:

   return 1;
}

static int
process_result(SSL* ssl, int socket, int32_t output_format)
{
   struct json* read = NULL;

   if (pgmoneta_management_read_json(ssl, socket, &read))
   {
      goto error;
   }

   if (MANAGEMENT_OUTPUT_FORMAT_TEXT == output_format)
   {
      pgmoneta_json_print(read, FORMAT_TEXT);
   }
   else
   {
      pgmoneta_json_print(read, FORMAT_JSON);
   }

   pgmoneta_json_destroy(read);

   return 0;

error:

   pgmoneta_json_destroy(read);

   return 1;
}
