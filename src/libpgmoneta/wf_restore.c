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
#include <deque.h>
#include <info.h>
#include <logging.h>
#include <restore.h>
#include <string.h>
#include <utils.h>
#include <workers.h>
#include <workflow.h>

/* system */
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

static int restore_setup(int, char*, struct deque*);
static int restore_execute(int, char*, struct deque*);
static int restore_teardown(int, char*, struct deque*);

static int recovery_info_setup(int, char*, struct deque*);
static int recovery_info_execute(int, char*, struct deque*);
static int recovery_info_teardown(int, char*, struct deque*);

static int restore_excluded_files_setup(int, char*, struct deque*);
static int restore_excluded_files_execute(int, char*, struct deque*);
static int restore_excluded_files_teardown(int, char*, struct deque*);

static char* get_user_password(char* username);
static void create_standby_signal(char* basedir);

struct workflow*
pgmoneta_workflow_create_restore(void)
{
   struct workflow* wf = NULL;

   wf = (struct workflow*)malloc(sizeof(struct workflow));

   if (wf == NULL)
   {
      return NULL;
   }

   wf->setup = &restore_setup;
   wf->execute = &restore_execute;
   wf->teardown = &restore_teardown;
   wf->next = NULL;

   return wf;
}

struct workflow*
pgmoneta_workflow_create_recovery_info(void)
{
   struct workflow* wf = NULL;

   wf = (struct workflow*)malloc(sizeof(struct workflow));

   if (wf == NULL)
   {
      return NULL;
   }

   wf->setup = &recovery_info_setup;
   wf->execute = &recovery_info_execute;
   wf->teardown = &recovery_info_teardown;
   wf->next = NULL;

   return wf;
}

struct workflow*
pgmoneta_restore_excluded_files(void)
{
   struct workflow* wf = NULL;

   wf = (struct workflow*)malloc(sizeof(struct workflow));

   if (wf == NULL)
   {
      return NULL;
   }

   wf->setup = &restore_excluded_files_setup;
   wf->execute = &restore_excluded_files_execute;
   wf->teardown = &restore_excluded_files_teardown;
   wf->next = NULL;

   return wf;
}

static int
restore_setup(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Restore (setup): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static int
restore_execute(int server, char* identifier, struct deque* nodes)
{
   char* position = NULL;
   char* directory = NULL;
   char* o = NULL;
   char* ident = NULL;
   int number_of_backups = 0;
   struct backup** backups = NULL;
   struct backup* backup = NULL;
   struct backup* verify = NULL;
   char* d = NULL;
   char* root = NULL;
   char* base = NULL;
   char* from = NULL;
   char* to = NULL;
   char* id = NULL;
   char* origwal = NULL;
   char* waldir = NULL;
   char* waltarget = NULL;
   int number_of_workers = 0;
   struct workers* workers = NULL;
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Restore (execute): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   position = (char*)pgmoneta_deque_get(nodes, "position");
   directory = (char*)pgmoneta_deque_get(nodes, "directory");

   if (!strcmp(identifier, "oldest"))
   {
      d = pgmoneta_get_server_backup(server);

      if (pgmoneta_get_backups(d, &number_of_backups, &backups))
      {
         goto error;
      }

      for (int i = 0; id == NULL && i < number_of_backups; i++)
      {
         if (backups[i]->valid == VALID_TRUE)
         {
            id = backups[i]->label;
         }
      }
   }
   else if (!strcmp(identifier, "latest") || !strcmp(identifier, "newest"))
   {
      d = pgmoneta_get_server_backup(server);

      if (pgmoneta_get_backups(d, &number_of_backups, &backups))
      {
         goto error;
      }

      for (int i = number_of_backups - 1; id == NULL && i >= 0; i--)
      {
         if (backups[i]->valid == VALID_TRUE)
         {
            id = backups[i]->label;
         }
      }
   }
   else
   {
      id = identifier;
   }

   if (id == NULL)
   {
      pgmoneta_log_error("Restore: No identifier for %s/%s", config->servers[server].name, identifier);
      goto error;
   }

   root = pgmoneta_get_server_backup(server);

   base = pgmoneta_get_server_backup_identifier(server, id);

   if (!pgmoneta_exists(base))
   {
      if (pgmoneta_get_backups(root, &number_of_backups, &backups))
      {
         goto error;
      }

      bool prefix_found = false;

      for (int i = 0; i < number_of_backups; i++)
      {
         if (backups[i]->valid == VALID_TRUE && pgmoneta_starts_with(backups[i]->label, id))
         {
            prefix_found = true;
            id = backups[i]->label;
            break;
         }
      }

      if (!prefix_found)
      {
         pgmoneta_log_error("Restore: Unknown identifier for %s/%s", config->servers[server].name, id);
         goto error;
      }
   }

   if (pgmoneta_get_backup(root, id, &verify))
   {
      pgmoneta_log_error("Restore: Unable to get backup for %s/%s", config->servers[server].name, id);
      goto error;
   }

   if (!verify->valid)
   {
      pgmoneta_log_error("Restore: Invalid backup for %s/%s", config->servers[server].name, id);
      goto error;
   }

   if (pgmoneta_deque_add(nodes, "root", (uintptr_t)directory, ValueString))
   {
      goto error;
   }

   from = pgmoneta_get_server_backup_identifier_data(server, id);

   to = pgmoneta_append(to, directory);
   if (!pgmoneta_ends_with(to, "/"))
   {
      to = pgmoneta_append(to, "/");
   }
   to = pgmoneta_append(to, config->servers[server].name);
   to = pgmoneta_append(to, "-");
   to = pgmoneta_append(to, id);
   to = pgmoneta_append(to, "/");

   pgmoneta_delete_directory(to);

   number_of_workers = pgmoneta_get_number_of_workers(server);
   if (number_of_workers > 0)
   {
      pgmoneta_workers_initialize(number_of_workers, &workers);
   }

   if (pgmoneta_copy_postgresql_restore(from, to, directory, config->servers[server].name, id, verify, workers))
   {
      pgmoneta_log_error("Restore: Could not restore %s/%s", config->servers[server].name, id);
      goto error;
   }
   else
   {
      if (position != NULL && strlen(position) > 0)
      {
         char tokens[512];
         bool primary = true;
         bool copy_wal = false;
         char ver[MISC_LENGTH] = {0};
         char* ptr = NULL;

         memset(&tokens[0], 0, sizeof(tokens));
         memcpy(&tokens[0], position, strlen(position));

         ptr = strtok(&tokens[0], ",");

         while (ptr != NULL)
         {
            char key[256];
            char value[256];
            char* equal = NULL;

            memset(&key[0], 0, sizeof(key));
            memset(&value[0], 0, sizeof(value));

            equal = strchr(ptr, '=');

            if (equal == NULL)
            {
               memcpy(&key[0], ptr, strlen(ptr));
            }
            else
            {
               memcpy(&key[0], ptr, strlen(ptr) - strlen(equal));
               memcpy(&value[0], equal + 1, strlen(equal) - 1);
            }

            if (!strcmp(&key[0], "current") ||
                !strcmp(&key[0], "immediate") ||
                !strcmp(&key[0], "name") ||
                !strcmp(&key[0], "xid") ||
                !strcmp(&key[0], "lsn") ||
                !strcmp(&key[0], "time"))
            {
               copy_wal = true;
            }
            else if (!strcmp(&key[0], "primary"))
            {
               primary = true;
            }
            else if (!strcmp(&key[0], "replica"))
            {
               primary = false;
            }
            else if (!strcmp(&key[0], "inclusive") || !strcmp(&key[0], "timeline") || !strcmp(&key[0], "action"))
            {
               /* Ok */
            }

            ptr = strtok(NULL, ",");
         }

         pgmoneta_get_backup(root, id, &backup);

         if (pgmoneta_deque_add(nodes, "primary", primary, ValueBool))
         {
            goto error;
         }

         snprintf(&ver[0], sizeof(ver), "%d", backup->major_version);
         if (pgmoneta_deque_add(nodes, "version", (uintptr_t)ver, ValueString))
         {
            goto error;
         }

         if (pgmoneta_deque_add(nodes, "recovery info", true, ValueBool))
         {
            goto error;
         }

         if (copy_wal)
         {
            origwal = pgmoneta_get_server_backup_identifier_data_wal(server, id);
            waldir = pgmoneta_get_server_wal(server);

            waltarget = pgmoneta_append(waltarget, directory);
            waltarget = pgmoneta_append(waltarget, "/");
            waltarget = pgmoneta_append(waltarget, config->servers[server].name);
            waltarget = pgmoneta_append(waltarget, "-");
            waltarget = pgmoneta_append(waltarget, id);
            waltarget = pgmoneta_append(waltarget, "/pg_wal/");

            pgmoneta_copy_wal_files(waldir, waltarget, &backup->wal[0], workers);
         }
      }
      else
      {
         if (pgmoneta_deque_add(nodes, "recovery info", false, ValueBool))
         {
            goto error;
         }
      }

      if (pgmoneta_deque_add(nodes, "to", (uintptr_t)to, ValueString))
      {
         goto error;
      }
   }

   if (number_of_workers > 0)
   {
      pgmoneta_workers_wait(workers);
      pgmoneta_workers_destroy(workers);
   }

   o = pgmoneta_append(o, directory);
   o = pgmoneta_append(o, "/");

   ident = pgmoneta_append(ident, id);

   if (pgmoneta_deque_add(nodes, "output", (uintptr_t)o, ValueString))
   {
      goto error;
   }

   if (pgmoneta_deque_add(nodes, "identifier", (uintptr_t)ident, ValueString))
   {
      goto error;
   }

   for (int i = 0; i < number_of_backups; i++)
   {
      free(backups[i]);
   }
   free(backups);

   free(backup);
   free(verify);
   free(root);
   free(base);
   free(from);
   free(d);
   free(to);
   free(o);
   free(ident);
   free(origwal);
   free(waldir);
   free(waltarget);

   return 0;

error:

   if (number_of_workers > 0)
   {
      pgmoneta_workers_wait(workers);
      pgmoneta_workers_destroy(workers);
   }

   for (int i = 0; i < number_of_backups; i++)
   {
      free(backups[i]);
   }
   free(backups);

   free(backup);
   free(verify);
   free(root);
   free(base);
   free(from);
   free(d);
   free(to);
   free(o);
   free(ident);
   free(origwal);
   free(waldir);
   free(waltarget);

   return 1;
}

static int
restore_teardown(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Restore (teardown): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static int
recovery_info_setup(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Recovery (setup): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static int
recovery_info_execute(int server, char* identifier, struct deque* nodes)
{
   char* base = NULL;
   char* position = NULL;
   bool primary;
   bool is_recovery_info;
   char tokens[256];
   char buffer[256];
   char line[1024];
   char* f = NULL;
   FILE* ffile = NULL;
   char* t = NULL;
   FILE* tfile = NULL;
   char* path = NULL;
   bool mode = false;
   char* ptr = NULL;
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Recovery (execute): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   is_recovery_info = (bool)pgmoneta_deque_get(nodes, "recovery info");

   if (!is_recovery_info)
   {
      goto done;
   }

   base = (char*)pgmoneta_deque_get(nodes, "to");

   if (base == NULL)
   {
      goto error;
   }

   position = (char*)pgmoneta_deque_get(nodes, "position");
   primary = (bool)pgmoneta_deque_get(nodes, "primary");

   if (!primary)
   {
      f = pgmoneta_append(f, base);
      if (!pgmoneta_ends_with(f, "/"))
      {
         f = pgmoneta_append(f, "/");
      }
      f = pgmoneta_append(f, "postgresql.conf");

      t = pgmoneta_append(t, f);
      t = pgmoneta_append(t, ".tmp");

      if (pgmoneta_exists(f))
      {
         ffile = fopen(f, "r");
      }
      else
      {
         pgmoneta_log_error("%s does not exists", f);
         goto error;
      }

      tfile = fopen(t, "w");

      if (tfile == NULL)
      {
         pgmoneta_log_error("Could not create %s", t);
         goto error;
      }

      if (ffile != NULL)
      {
         while ((fgets(&buffer[0], sizeof(buffer), ffile)) != NULL)
         {
            if (pgmoneta_starts_with(&buffer[0], "standby_mode") ||
                pgmoneta_starts_with(&buffer[0], "recovery_target") ||
                pgmoneta_starts_with(&buffer[0], "primary_conninfo") ||
                pgmoneta_starts_with(&buffer[0], "primary_slot_name"))
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "#%s", &buffer[0]);
               fputs(&line[0], tfile);
            }
            else
            {
               fputs(&buffer[0], tfile);
            }
         }
      }

      if (position != NULL)
      {
         memset(&tokens[0], 0, sizeof(tokens));
         memcpy(&tokens[0], position, strlen(position));

         memset(&line[0], 0, sizeof(line));
         snprintf(&line[0], sizeof(line), "#\n");
         fputs(&line[0], tfile);

         memset(&line[0], 0, sizeof(line));
         snprintf(&line[0], sizeof(line), "# Generated by pgmoneta\n");
         fputs(&line[0], tfile);

         memset(&line[0], 0, sizeof(line));
         snprintf(&line[0], sizeof(line), "#\n");
         fputs(&line[0], tfile);

         memset(&line[0], 0, sizeof(line));
         snprintf(&line[0], sizeof(line), "primary_conninfo = \'host=%s port=%d user=%s password=%s application_name=%s\'\n",
                  config->servers[server].host, config->servers[server].port, config->servers[server].username,
                  get_user_password(config->servers[server].username), config->servers[server].wal_slot);
         fputs(&line[0], tfile);

         memset(&line[0], 0, sizeof(line));
         snprintf(&line[0], sizeof(line), "primary_slot_name = \'%s\'\n", config->servers[server].wal_slot);
         fputs(&line[0], tfile);

         ptr = strtok(&tokens[0], ",");

         while (ptr != NULL)
         {
            char key[256];
            char value[256];
            char* equal = NULL;

            memset(&key[0], 0, sizeof(key));
            memset(&value[0], 0, sizeof(value));

            equal = strchr(ptr, '=');

            if (equal == NULL)
            {
               memcpy(&key[0], ptr, strlen(ptr));
            }
            else
            {
               memcpy(&key[0], ptr, strlen(ptr) - strlen(equal));
               memcpy(&value[0], equal + 1, strlen(equal) - 1);
            }

            if (!strcmp(&key[0], "current") || !strcmp(&key[0], "immediate"))
            {
               if (!mode)
               {
                  memset(&line[0], 0, sizeof(line));
                  snprintf(&line[0], sizeof(line), "recovery_target = \'immediate\'\n");
                  fputs(&line[0], tfile);

                  mode = true;
               }
            }
            else if (!strcmp(&key[0], "name"))
            {
               if (!mode)
               {
                  memset(&line[0], 0, sizeof(line));
                  snprintf(&line[0], sizeof(line), "recovery_target_name = \'%s\'\n", strlen(value) > 0 ? &value[0] : "");
                  fputs(&line[0], tfile);

                  mode = true;
               }
            }
            else if (!strcmp(&key[0], "xid"))
            {
               if (!mode)
               {
                  memset(&line[0], 0, sizeof(line));
                  snprintf(&line[0], sizeof(line), "recovery_target_xid = \'%s\'\n", strlen(value) > 0 ? &value[0] : "");
                  fputs(&line[0], tfile);

                  mode = true;
               }
            }
            else if (!strcmp(&key[0], "lsn"))
            {
               if (!mode)
               {
                  memset(&line[0], 0, sizeof(line));
                  snprintf(&line[0], sizeof(line), "recovery_target_lsn = \'%s\'\n", strlen(value) > 0 ? &value[0] : "");
                  fputs(&line[0], tfile);

                  mode = true;
               }
            }
            else if (!strcmp(&key[0], "time"))
            {
               if (!mode)
               {
                  memset(&line[0], 0, sizeof(line));
                  snprintf(&line[0], sizeof(line), "recovery_target_time = \'%s\'\n", strlen(value) > 0 ? &value[0] : "");
                  fputs(&line[0], tfile);

                  mode = true;
               }
            }
            else if (!strcmp(&key[0], "primary") || !strcmp(&key[0], "replica"))
            {
               /* Ok */
            }
            else if (!strcmp(&key[0], "inclusive"))
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "recovery_target_inclusive = %s\n", strlen(value) > 0 ? &value[0] : "on");
               fputs(&line[0], tfile);
            }
            else if (!strcmp(&key[0], "timeline"))
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "recovery_target_timeline = \'%s\'\n", strlen(value) > 0 ? &value[0] : "latest");
               fputs(&line[0], tfile);
            }
            else if (!strcmp(&key[0], "action"))
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "recovery_target_action = \'%s\'\n", strlen(value) > 0 ? &value[0] : "pause");
               fputs(&line[0], tfile);
            }
            else
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "%s = \'%s\'\n", &key[0], strlen(value) > 0 ? &value[0] : "");
               fputs(&line[0], tfile);
            }

            ptr = strtok(NULL, ",");
         }
      }

      if (ffile != NULL)
      {
         fclose(ffile);
      }
      if (tfile != NULL)
      {
         fclose(tfile);
      }

      pgmoneta_move_file(t, f);

      create_standby_signal(base);
   }
   else
   {
      f = pgmoneta_append(f, base);
      if (!pgmoneta_ends_with(f, "/"))
      {
         f = pgmoneta_append(f, "/");
      }
      f = pgmoneta_append(f, "postgresql.auto.conf");

      t = pgmoneta_append(t, f);
      t = pgmoneta_append(t, ".tmp");

      if (pgmoneta_exists(f))
      {
         ffile = fopen(f, "r");
      }
      else
      {
         pgmoneta_log_error("%s does not exists", f);
         goto error;
      }

      tfile = fopen(t, "w");

      if (tfile == NULL)
      {
         pgmoneta_log_error("Could not create %s", t);
         goto error;
      }

      if (ffile != NULL)
      {
         while ((fgets(&buffer[0], sizeof(buffer), ffile)) != NULL)
         {
            if (pgmoneta_starts_with(&buffer[0], "primary_conninfo"))
            {
               memset(&line[0], 0, sizeof(line));
               snprintf(&line[0], sizeof(line), "#%s", &buffer[0]);
               fputs(&line[0], tfile);
            }
            else
            {
               fputs(&buffer[0], tfile);
            }
         }
      }

      if (ffile != NULL)
      {
         fclose(ffile);
      }
      if (tfile != NULL)
      {
         fclose(tfile);
      }

      pgmoneta_move_file(t, f);

      path = pgmoneta_append(path, base);
      if (!pgmoneta_ends_with(path, "/"))
      {
         path = pgmoneta_append(path, "/");
      }
      path = pgmoneta_append(path, "standby.signal");

      if (pgmoneta_exists(path))
      {
         pgmoneta_delete_file(path, NULL);
      }
   }

done:

   free(f);
   free(t);
   free(path);

   return 0;

error:

   if (ffile != NULL)
   {
      fclose(ffile);
   }
   if (tfile != NULL)
   {
      fclose(tfile);
   }

   free(f);
   free(t);
   free(path);

   return 1;
}

static int
recovery_info_teardown(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Recovery (teardown): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static int
restore_excluded_files_setup(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Excluded (setup): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static int
restore_excluded_files_execute(int server, char* identifier, struct deque* nodes)
{
   char* id = NULL;
   char* from = NULL;
   char* to = NULL;
   int number_of_backups = 0;
   struct backup** backups = NULL;
   char* d = NULL;
   struct workers* workers = NULL;
   int number_of_workers = 0;
   char** restore_last_files_names = NULL;
   char* directory = NULL;
   struct configuration* config = (struct configuration*)shmem;

   pgmoneta_log_debug("Excluded (execute): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   if (pgmoneta_get_restore_last_files_names(&restore_last_files_names))
   {
      goto error;
   }

   if (!strcmp(identifier, "oldest"))
   {
      d = pgmoneta_get_server_backup(server);

      if (pgmoneta_get_backups(d, &number_of_backups, &backups))
      {
         goto error;
      }

      for (int i = 0; id == NULL && i < number_of_backups; i++)
      {
         if (backups[i]->valid == VALID_TRUE)
         {
            id = backups[i]->label;
         }
      }
   }
   else if (!strcmp(identifier, "latest") || !strcmp(identifier, "newest"))
   {
      d = pgmoneta_get_server_backup(server);

      if (pgmoneta_get_backups(d, &number_of_backups, &backups))
      {
         goto error;
      }

      for (int i = number_of_backups - 1; id == NULL && i >= 0; i--)
      {
         if (backups[i]->valid == VALID_TRUE)
         {
            id = backups[i]->label;
         }
      }
   }
   else
   {
      id = identifier;
   }

   directory = (char*)pgmoneta_deque_get(nodes, "directory");

   from = pgmoneta_get_server_backup_identifier_data(server, id);

   to = pgmoneta_append(to, directory);
   if (!pgmoneta_ends_with(to, "/"))
   {
      to = pgmoneta_append(to, "/");
   }
   to = pgmoneta_append(to, config->servers[server].name);
   to = pgmoneta_append(to, "-");
   to = pgmoneta_append(to, id);
   to = pgmoneta_append(to, "/");

   for (int i = 0; restore_last_files_names[i] != NULL; i++)
   {
      char* from_file = NULL;
      char* to_file = NULL;

      from_file = (char*)malloc((strlen(from) + strlen(restore_last_files_names[i])) * sizeof(char) + 1);
      if (from_file == NULL)
      {
         pgmoneta_log_error("Restore: Could not allocate memory for from_file");
         goto error;
      }
      from_file = strcpy(from_file, from);
      from_file = strcat(from_file, restore_last_files_names[i]);

      to_file = (char*)malloc((strlen(to) + strlen(restore_last_files_names[i])) * sizeof(char) + 1);
      if (to_file == NULL)
      {
         pgmoneta_log_error("Restore: Could not allocate memory for to_file");
         free(from_file);
         goto error;
      }

      to_file = strcpy(to_file, to);
      to_file = strcat(to_file, restore_last_files_names[i]);

      number_of_workers = pgmoneta_get_number_of_workers(server);
      if (number_of_workers > 0)
      {
         pgmoneta_workers_initialize(number_of_workers, &workers);
      }

      if (pgmoneta_copy_file(from_file, to_file, workers))
      {
         pgmoneta_log_error("Restore: Could not copy file %s to %s", from_file, to_file);
         free(from_file);
         free(to_file);
         goto error;
      }
      free(from_file);
      free(to_file);
   }

   for (int i = 0; i < number_of_backups; i++)
   {
      free(backups[i]);
   }
   for (int i = 0; restore_last_files_names[i] != NULL; i++)
   {
      free(restore_last_files_names[i]);
   }
   free(backups);
   free(restore_last_files_names);
   free(from);
   free(to);
   free(d);

   return 0;

error:
   for (int i = 0; i < number_of_backups; i++)
   {
      free(backups[i]);
   }
   for (int i = 0; restore_last_files_names[i] != NULL; i++)
   {
      free(restore_last_files_names[i]);
   }
   free(backups);
   free(restore_last_files_names);
   free(from);
   free(to);
   free(d);

   return 1;
}

static int
restore_excluded_files_teardown(int server, char* identifier, struct deque* nodes)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   pgmoneta_log_debug("Excluded (teardown): %s/%s", config->servers[server].name, identifier);
   pgmoneta_deque_list(nodes);

   return 0;
}

static char*
get_user_password(char* username)
{
   struct configuration* config;

   config = (struct configuration*)shmem;

   for (int i = 0; i < config->number_of_users; i++)
   {
      if (!strcmp(&config->users[i].username[0], username))
      {
         return &config->users[i].password[0];
      }
   }

   return NULL;
}

static void
create_standby_signal(char* basedir)
{
   char* f = NULL;
   int fd = 0;

   f = pgmoneta_append(f, basedir);
   if (!pgmoneta_ends_with(f, "/"))
   {
      f = pgmoneta_append(f, "/");
   }
   f = pgmoneta_append(f, "standby.signal");

   if ((fd = creat(f, 0600)) < 0)
   {
      pgmoneta_log_error("Unable to create: %s", f);
   }

   if (fd > 0)
   {
      close(fd);
   }

   free(f);
}
