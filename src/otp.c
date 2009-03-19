/* OpenVAS Manager
 * $Id$
 * Description: Module for OpenVAS Manager: the OTP library.
 *
 * Authors:
 * Matthew Mundell <matt@mundell.ukfsn.org>
 *
 * Copyright:
 * Copyright (C) 2008, 2009 Intevation GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * or, at your option, any later version as published by the Free
 * Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file  otp.c
 * @brief The OpenVAS Manager OTP library.
 *
 * This file defines an OpenVAS Transfer Protocol (OTP) library, for
 * implementing OpenVAS managers such as the OpenVAS Manager daemon.
 *
 * The library provides a single function, \ref process_otp_server_input.
 * This function parses a given string of OTP text and adjusts local
 * task records according to the OTP messages in the string.
 */

#include "otp.h"
#include "manage.h"
#include "string.h"
#include "tracef.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FIX Should probably be passed into process_otp_server_input.
extern int from_buffer_size;


/* Helper functions. */

/**
 * @brief Free a GSList.
 *
 * Wrapper for GHashTable.
 *
 * @param[in]  list  A pointer to a GSList.
 */
void
free_g_slist (gpointer list)
{
  g_slist_free ((GSList*) list);
}


/* Ports. */

// FIX currently in manage.h
#if 0
/**
 * @brief Possible port types.
 */
typedef enum
{
  PORT_PROTOCOL_TCP,
  PORT_PROTOCOL_UDP,
  PORT_PROTOCOL_OTHER
} port_protocol_t;

/**
 * @brief A port.
 */
typedef struct
{
  int number;                ///< Port number.
  port_protocol_t protocol;  ///< Port protocol (TCP, UDP, ...).
} port_t;
#endif

/**
 * @brief Get the name of the protocol of a port.
 *
 * @param[in]  port  The port.
 *
 * @return The name.
 */
char*
port_protocol_name (port_t* port)
{
  switch (port->protocol)
    {
      case PORT_PROTOCOL_TCP: return "tcp";
      case PORT_PROTOCOL_UDP: return "udp";
      case PORT_PROTOCOL_OTHER: return "???";
      default: assert (0); return "";
    }
}

/**
 * @brief Print a string representation of a port to a stream.
 *
 * @param[in]  stream  Destination stream.
 * @param[in]  port    Port to print.
 */
void
print_port (FILE* stream, port_t* port)
{
  fprintf (stream, "FIX (%d/%s)", port->number, port_protocol_name (port));
}


/* Messages. */

// FIX should be in manage.c
#if 0
/**
 * @brief The record of a message.
 */
typedef struct
{
  port_t port;          ///< The port.
  char* description;    ///< Description of the message.
  char* oid;            ///< NVT identifier.
} message_t;
#endif

/**
 * @brief Current message during OTP SERVER message commands.
 */
message_t* current_message = NULL;

/**
 * @brief Make a message.
 *
 * @param[in]  number    Port number.
 * @param[in]  protocol  Port protocol.
 *
 * @return A pointer to the new message.
 */
message_t*
make_message (unsigned int number, const char* protocol)
{
  tracef ("   make_message %u %s\n", number, protocol);

  message_t* message = g_malloc (sizeof (message_t));

  message->description = NULL;
  message->oid = NULL;
  message->port.number = number;
  if (strncasecmp ("udp", protocol, 3) == 0)
    message->port.protocol = PORT_PROTOCOL_UDP;
  else if (strncasecmp ("tcp", protocol, 3) == 0)
    message->port.protocol = PORT_PROTOCOL_TCP;
  else
    message->port.protocol = PORT_PROTOCOL_OTHER;

  return message;
}

#if 0
// FIX currently in manage.c
/**
 * @brief Free a message for g_ptr_array_foreach.
 *
 * @param[in]  message       Pointer to the message.
 * @param[in]  dummy         Dummy parameter.
 */
void
free_message (gpointer message, gpointer dummy)
{
  message_t* msg = (message_t*) message;
  if (msg->description) free (msg->description);
  if (msg->oid) free (msg->oid);
  free (msg);
}
#endif

/**
 * @brief Set the description of a message.
 *
 * @param[in]  message       Pointer to the message.
 * @param[in]  description  Description.
 */
void
set_message_description (message_t* message, char* description)
{
  if (message->description) free (message->description);
  message->description = description;
}

/**
 * @brief Set the OID of a message.
 *
 * @param[in]  message       Pointer to the message.
 * @param[in]  oid          OID.
 */
void
set_message_oid (message_t* message, char* oid)
{
  if (message->oid) free (message->oid);
  message->oid = oid;
}

/**
 * @brief Pair of stream and type for write_messages.
 */
typedef struct
{
  FILE* stream;  ///< Destination stream.
  char* type;    ///< Type of message.
} message_data_t;

/**
 * @brief Write a message for g_ptr_array_foreach.
 *
 * @param[in]  message       The message.
 * @param[in]  message_data  The stream and message type.
 */
void
write_message (gpointer message, gpointer message_data)
{
  message_t* msg = (message_t*) message;
  message_data_t* data = (message_data_t*) message_data;
  fprintf (data->stream, "results|%s|%s|", "dik", "dik"); // FIX
  print_port (data->stream, &msg->port);
  fprintf (data->stream, "|%s|%s|%s|\n",
           msg->oid, data->type, msg->description);
}

/**
 * @brief Write an array of messages to a stream.
 *
 * @param[in]  file       The stream.
 * @param[in]  messages   Array of messages.
 * @param[in]  type       Type of message.
 */
void
write_messages (FILE* file, GPtrArray* messages, char* type)
{
  message_data_t data = { file, type};
  g_ptr_array_foreach (messages, write_message, &data);
}

/**
 * @brief Write a timestamp to a stream.
 *
 * @param[in]  file       The stream.
 * @param[in]  type       Type of timestamp.
 * @param[in]  time       The time.
 */
void
write_timestamp (FILE* file, char* type, char* time)
{
  fprintf (file, "timestamps|%s|%s|%s|%s|\n", "dik", "dik", type, time); // FIX
}

/**
 * @brief Save a report to a file.
 *
 * @param[in]  task       The task.
 *
 * @return 0 success, -1 failed to open file, -2 failed to close file,
 *         -3 failed to symlink file to task dir, -4 failed to create dir,
 *         -5 failed to generate ID.
 */
int
save_report (task_t* task)
{
  const char* id;

  if (current_credentials.username == NULL) return -1;

  tracef ("   Saving report %s on task %u\n", task->start_time, task->id);

  if (task_id_string (task, &id)) return -1;

  gchar* user_dir_name = g_build_filename (PREFIX
                                           "/var/lib/openvas/mgr/users/",
                                           current_credentials.username,
                                           "reports",
                                           NULL);

  /* Ensure user reports directory exists. */

  if (g_mkdir_with_parents (user_dir_name, 33216 /* -rwx------ */) == -1)
    {
      fprintf (stderr, "Failed to create report dir %s: %s\n",
               user_dir_name,
               strerror (errno));
      g_free (user_dir_name);
      return -4;
    }

  /* Generate report directory name. */

  // FIX OID
  static char buffer[11]; /* (expt 2 32) => 4294967296 */
  int length = sprintf (buffer, "%010u", task->report_count);
  assert (length < 15);
  if (length < 4)
    {
      fprintf (stderr, "Failed to generate report id.\n");
      g_free (user_dir_name);
      return -5;
    }

  gchar* dir_name = g_build_filename (PREFIX
                                      "/var/lib/openvas/mgr/users/",
                                      current_credentials.username,
                                      "tasks",
                                      id,
                                      "reports",
                                      buffer,
                                      NULL);

  gchar* symlink_name = g_build_filename (user_dir_name, buffer, NULL);
  g_free (user_dir_name);

  /* Ensure task report directory exists. */

  if (g_mkdir_with_parents (dir_name, 33216 /* -rwx------ */) == -1)
    {
      fprintf (stderr, "Failed to create report dir %s: %s\n",
               dir_name,
               strerror (errno));
      g_free (dir_name);
      g_free (symlink_name);
      return -4;
    }

  /* Link report directory into task directory. */

  if (symlink (dir_name, symlink_name))
    {
      rmdir (dir_name);
      fprintf (stderr, "Failed to symlink %s to %s\n",
               dir_name,
               symlink_name);
      g_free (dir_name);
      g_free (symlink_name);
      return -3;
    }

  /* Write report. */

  gchar* name = g_build_filename (dir_name, "report.nbe", NULL);

  FILE* file = fopen (name, "w");
  if (file == NULL)
    {
      rmdir (dir_name);
      fprintf (stderr, "Failed to open report file %s: %s\n",
               name,
               strerror (errno));
      g_free (dir_name);
      g_free (name);
      g_free (symlink_name);
      return -1;
    }

  write_timestamp (file, "scan_start", task->start_time); // FIX
  write_timestamp (file, "host_start", task->start_time);

  //write_messages (file, task->open_ports, task->open_ports_size); FIX
  write_messages (file, task->debugs, "Debug Message");
  write_messages (file, task->holes, "Security Hole");
  write_messages (file, task->infos, "Security Warning");
  write_messages (file, task->logs, "Log Message");
  write_messages (file, task->notes, "Security Note");

  write_timestamp (file, "host_end", task->end_time);
  write_timestamp (file, "scan_end", task->end_time); // FIX

  if (fclose (file))
    {
      unlink (name);
      rmdir (dir_name);
      fprintf (stderr, "Failed to close report file %s: %s\n",
               name,
               strerror (errno));
      g_free (dir_name);
      g_free (name);
      g_free (symlink_name);
      return -2;
    }

  task->report_count++;

  g_free (dir_name);
  g_free (name);
  g_free (symlink_name);
  return 0;
}


/* Appending messages to tasks. */  // FIX ... to reports.

/**
 * @brief Append a debug message to a task.
 *
 * @param[in]  task         Task.
 * @param[in]  message      Message.
 */
void
append_debug_message (task_t* task, message_t* message)
{
  g_ptr_array_add (task->debugs, (gpointer) message);
  task->debugs_size++;
}

/**
 * @brief Append a hole message to a task.
 *
 * @param[in]  task         Task.
 * @param[in]  message      Message.
 */
void
append_hole_message (task_t* task, message_t* message)
{
  g_ptr_array_add (task->holes, (gpointer) message);
  task->holes_size++;
}

/**
 * @brief Append an info message to a task.
 *
 * @param[in]  task         Task.
 * @param[in]  message      Message.
 */
void
append_info_message (task_t* task, message_t* message)
{
  g_ptr_array_add (task->infos, (gpointer) message);
  task->infos_size++;
}

/**
 * @brief Append a log message to a task.
 *
 * @param[in]  task         Task.
 * @param[in]  message      Message.
 */
void
append_log_message (task_t* task, message_t* message)
{
  g_ptr_array_add (task->logs, (gpointer) message);
  task->logs_size++;
}

/**
 * @brief Append a note message to a task.
 *
 * @param[in]  task         Task.
 * @param[in]  message      Message.
 */
void
append_note_message (task_t* task, message_t* message)
{
  g_ptr_array_add (task->notes, (gpointer) message);
  task->notes_size++;
}


/* Server preferences. */

/**
 * @brief The current server preference, during reading of server preferences.
 */
char* current_server_preference = NULL;

/**
 * @brief Free any server preferences.
 */
void
maybe_free_server_preferences ()
{
  if (server.preferences) g_hash_table_destroy (server.preferences);
}

/**
 * @brief Create the server preferences.
 */
void
make_server_preferences ()
{
  server.preferences = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_free);
}

/**
 * @brief Add a preference to the server preferences.
 *
 * Both parameters are used directly, and are freed when the
 * preferences are freed.
 *
 * @param[in]  preference  The preference.
 * @param[in]  value       The value of the preference.
 */
void
add_server_preference (char* preference, char* value)
{
  g_hash_table_insert (server.preferences, preference, value);
}


/* Server plugin dependencies. */

/**
 * @brief The current server plugin, during reading of server plugin dependencies.
 */
char* current_server_plugin_dependency_name = NULL;

/**
 * @brief The plugins required by the current server plugin.
 */
GSList* current_server_plugin_dependency_dependencies = NULL;

/**
 * @brief Free any server plugins dependencies.
 */
void
maybe_free_server_plugins_dependencies ()
{
  if (server.plugins_dependencies)
    {
      g_hash_table_destroy (server.plugins_dependencies);
      server.plugins_dependencies = NULL;
    }
}

/**
 * @brief Make the server plugins dependencies.
 */
void
make_server_plugins_dependencies ()
{
  assert (server.plugins_dependencies == NULL);
  server.plugins_dependencies = g_hash_table_new_full (g_str_hash,
                                                       g_str_equal,
                                                       g_free,
                                                       free_g_slist);
}

/**
 * @brief Add a plugin to the server dependencies.
 *
 * @param[in]  name          The name of the plugin.
 * @param[in]  requirements  The plugins required by the plugin.
 */
void
add_server_plugins_dependency (char* name, GSList* requirements)
{
  assert (server.plugins_dependencies);
  tracef ("   server new dependency name: %s\n", name);
  g_hash_table_insert (server.plugins_dependencies, name, requirements);
}

/**
 * @brief Set the current plugin.
 *
 * @param[in]  name  The name of the plugin.
 */
void
make_current_server_plugin_dependency (char* name)
{
  assert (current_server_plugin_dependency_name == NULL);
  assert (current_server_plugin_dependency_dependencies == NULL);
  current_server_plugin_dependency_name = name;
  current_server_plugin_dependency_dependencies = NULL; /* Empty list. */
}

/**
 * @brief Append a requirement to the current plugin.
 *
 * @param[in]  requirement  The name of the required plugin.
 */
void
append_to_current_server_plugin_dependency (char* requirement)
{
  tracef ("   server appending plugin requirement: %s\n", requirement);
  current_server_plugin_dependency_dependencies
    = g_slist_append (current_server_plugin_dependency_dependencies,
                      requirement);
}

/**
 * @brief Free any current server plugin dependency information.
 */
void
maybe_free_current_server_plugin_dependency ()
{
  if (current_server_plugin_dependency_name)
    free (current_server_plugin_dependency_name);
  if (current_server_plugin_dependency_dependencies)
    g_slist_free (current_server_plugin_dependency_dependencies);
}

/**
 * @brief Add the current plugin to the server dependencies.
 */
void
finish_current_server_plugin_dependency ()
{
  assert (current_server_plugin_dependency_name);
  add_server_plugins_dependency (current_server_plugin_dependency_name,
                                 current_server_plugin_dependency_dependencies);
  current_server_plugin_dependency_name = NULL;
  current_server_plugin_dependency_dependencies = NULL;
}


/* Server rules. */

/**
 * @brief Free a server rule.
 *
 * @param[in]  rule   The server rule.
 * @param[in]  dummy  Dummy parameter, to please g_ptr_array_foreach.
 */
void
free_rule (void* rule, void* dummy)
{
  free (rule);
}

/**
 * @brief Free any server rules.
 */
void
maybe_free_server_rules ()
{
  if (server.rules)
    {
      g_ptr_array_foreach (server.rules, free_rule, NULL);
      g_ptr_array_free (server.rules, TRUE);
      server.rules_size = 0;
    }
}

/**
 * @brief Create the server rules.
 */
void
make_server_rules ()
{
  server.rules = g_ptr_array_new ();
  server.rules_size = 0;
}

/**
 * @brief Add a rule to the server rules.
 *
 * The rule is used directly (versus using a copy) and is freed with the
 * other server rules.
 *
 * @param[in]  rule  The rule.
 */
void
add_server_rule (char* rule)
{
  g_ptr_array_add (server.rules, rule);
  server.rules_size++;
}


/* Server state. */

/**
 * @brief Information about the server.
 */
server_t server;

/**
 * @brief Initialise OTP library data.
 *
 * This must run once, before the first call to \ref process_otp_server_input.
 */
void
init_otp_data ()
{
  server.preferences = NULL;
  server.rules = NULL;
}

/**
 * @brief Possible states of the server.
 */
typedef enum
{
  SERVER_BYE,
  SERVER_DONE,
  SERVER_DEBUG_DESCRIPTION,
  SERVER_DEBUG_HOST,
  SERVER_DEBUG_NUMBER,
  SERVER_DEBUG_OID,
  SERVER_HOLE_DESCRIPTION,
  SERVER_HOLE_HOST,
  SERVER_HOLE_NUMBER,
  SERVER_HOLE_OID,
  SERVER_INFO_DESCRIPTION,
  SERVER_INFO_HOST,
  SERVER_INFO_NUMBER,
  SERVER_INFO_OID,
  SERVER_LOG_DESCRIPTION,
  SERVER_LOG_HOST,
  SERVER_LOG_NUMBER,
  SERVER_LOG_OID,
  SERVER_NOTE_DESCRIPTION,
  SERVER_NOTE_HOST,
  SERVER_NOTE_NUMBER,
  SERVER_NOTE_OID,
  SERVER_PLUGINS_MD5,
  SERVER_PLUGIN_DEPENDENCY_NAME,
  SERVER_PLUGIN_DEPENDENCY_DEPENDENCY,
  SERVER_PORT_HOST,
  SERVER_PORT_NUMBER,
  SERVER_PREFERENCE_NAME,
  SERVER_PREFERENCE_VALUE,
  SERVER_RULE,
  SERVER_SERVER,
  SERVER_STATUS,
  SERVER_STATUS_ATTACK_STATE,
  SERVER_STATUS_HOST,
  SERVER_STATUS_PORTS,
  SERVER_TIME,
  SERVER_TIME_HOST_START_HOST,
  SERVER_TIME_HOST_START_TIME,
  SERVER_TIME_HOST_END_HOST,
  SERVER_TIME_HOST_END_TIME,
  SERVER_TIME_SCAN_START,
  SERVER_TIME_SCAN_END,
  SERVER_TOP
} server_state_t;

/**
 * @brief The state of the server.
 */
server_state_t server_state = SERVER_TOP;

/**
 * @brief Set the server state, \ref server_state.
 */
void
set_server_state (server_state_t state)
{
  server_state = state;
  tracef ("   server state set: %i\n", server_state);
}

/**
 * @brief The initialisation state of the server.
 */
server_init_state_t server_init_state = SERVER_INIT_TOP;

/**
 * @brief Offset into initialisation string being sent to server.
 */
int server_init_offset = 0;

/**
 * @brief Set the server initialisation state, \ref server_init_state.
 */
void
set_server_init_state (server_init_state_t state)
{
  server_init_state = state;
  tracef ("   server init state set: %i\n", server_init_state);
}


/* OTP input processor. */

// FIX probably should pass to process_omp_client_input
extern char from_server[];
extern int from_server_start;
extern int from_server_end;

/**
 * @brief Parse the final SERVER field of an OTP message.
 *
 * @param  messages  A pointer into the OTP input buffer.
 *
 * @return 0 success, -1 fail, -2 too few characters (need more input).
 */
int
parse_server_done (char** messages)
{
  char *end = *messages + from_server_end - from_server_start;
  while (*messages < end && ((*messages)[0] == ' ' || (*messages)[0] == '\n'))
    { (*messages)++; from_server_start++; }
  if ((int) (end - *messages) < 6)
    /* Too few characters to be the end marker, return to select to
     * wait for more input. */
    return -2;
  if (strncasecmp ("SERVER", *messages, 6))
    {
      tracef ("   server fail: expected final \"SERVER\"\n");
      return -1;
    }
  set_server_state (SERVER_TOP);
  from_server_start += 6;
  (*messages) += 6;
  return 0;
}

/**
 * @brief Parse the final SERVER field of an OTP message.
 *
 * @param  messages  A pointer into the OTP input buffer.
 *
 * @return 0 success, -2 too few characters (need more input).
 */
int
parse_server_preference_value (char** messages)
{
  char *value, *end, *match;
  assert (current_server_preference);
  end = *messages + from_server_end - from_server_start;
  while (*messages < end && ((*messages)[0] == ' '))
    { (*messages)++; from_server_start++; }
  if ((match = memchr (*messages, '\n', from_server_end - from_server_start)))
    {
      match[0] = '\0';
      value = g_strdup (*messages);
      add_server_preference (current_server_preference, value);
      set_server_state (SERVER_PREFERENCE_NAME);
      from_server_start += match + 1 - *messages;
      *messages = match + 1;
      return 0;
    }
  return -2;
}

/**
 * @brief Parse an OTP rule.
 *
 * @param  messages  A pointer into the OTP input buffer.
 *
 * @return 0 read a rule, -1 read a <|>, -2 too few characters (need
 *         more input).
 */
int
parse_server_rule (char** messages)
{
  char *end, *match;
  end = *messages + from_server_end - from_server_start;
  while (*messages < end && ((*messages)[0] == '\n'))
    { (*messages)++; from_server_start++; }
  while (*messages < end && ((*messages)[0] == ' '))
    { (*messages)++; from_server_start++; }
  /* Check for the end marker. */
  if (end - *messages > 2
      && (*messages)[0] == '<'
      && (*messages)[1] == '|'
      && (*messages)[2] == '>')
    /* The rules list ends with "<|> SERVER". */
    return -1;
  /* There may be a rule ending in a semicolon. */
  if ((match = memchr (*messages, ';', from_server_end - from_server_start)))
    {
      char* rule;
      match[0] = '\0';
      rule = g_strdup (*messages);
      add_server_rule (rule);
      from_server_start += match + 1 - *messages;
      *messages = match + 1;
      return 0;
    }
  return -2;
}

/**
 * @brief Parse the dependency of a server plugin.
 *
 * @param  messages  A pointer into the OTP input buffer.
 *
 * @return 1 if a <|> follows in the buffer, otherwise 0.
 */
int
parse_server_plugin_dependency_dependency (char** messages)
{
  /* Look for the end of dependency marker: a newline that comes before
   * the next <|>. */
  char *separator, *end, *match, *input;
  int from_start, from_end;
  separator = NULL;
  /* Look for <|>. */
  input = *messages;
  from_start = from_server_start;
  from_end = from_server_end;
  while (from_start < from_end
         && (match = memchr (input, '<', from_end - from_start)))
    {
      if (((int) (match - input) - from_start + 1) < from_end
          && (match[1] == '|')
          && (match[2] == '>'))
        {
          separator = match;
          break;
        }
      from_start += match + 1 - input;
      input = match + 1;
    }
  /* Look for newline. */
  end = *messages + from_server_end - from_server_start;
  while (*messages < end && ((*messages)[0] == ' '))
    { (*messages)++; from_server_start++; }
  if ((match = memchr (*messages, '\n', from_server_end - from_server_start)))
    {
      /* Compare newline position to <|> position. */
      if ((separator == NULL) || (match < separator))
        {
          finish_current_server_plugin_dependency ();
          from_server_start += match + 1 - *messages;
          *messages = match + 1;
          set_server_state (SERVER_PLUGIN_DEPENDENCY_NAME);
        }
    }
  return separator == NULL;
}


/**
 * @brief Parse the field following "SERVER <|>".
 *
 * @param  messages  A pointer into the OTP input buffer.
 *
 * @return 0 found a newline delimited field, -1 error, -2 need more input,
 *         -3 found a <|> before next newline (that is, a <|> delimited
 *         field follows), -4 failed to find a newline (may be a <|>)
 */
int
parse_server_server (char** messages)
{
  char *end, *match;
  end = *messages + from_server_end - from_server_start;
  while (*messages < end && ((*messages)[0] == ' '))
    { (*messages)++; from_server_start++; }
  if ((match = memchr (*messages, '\n', from_server_end - from_server_start)))
    {
      match[0] = '\0';
      // FIX is there ever whitespace before the newline?
      while (*messages < end && ((*messages)[0] == ' '))
        { (*messages)++; from_server_start++; }
      // FIX 20 available?
      if (strncasecmp ("PLUGINS_DEPENDENCIES", *messages, 20) == 0)
        {
          from_server_start += match + 1 - *messages;
          *messages = match + 1;
          maybe_free_server_plugins_dependencies ();
          make_server_plugins_dependencies ();
          set_server_state (SERVER_PLUGIN_DEPENDENCY_NAME);
          return 0;
        }
      char* newline = match;
      newline[0] = '\n';
      /* Check for a <|>. */
      char* input = *messages;
      int from_start = from_server_start;
      int from_end = from_server_end;
      while (from_start < from_end
             && (match = memchr (input, '<', from_end - from_start)))
        {
          if ((((int) (match - input) - from_start + 1) < from_end)
              && (match[1] == '|')
              && (match[2] == '>'))
            {
              if (match > newline)
                /* The next <|> is after the newline, which is an error. */
                return -1;
              /* The next <|> is before the newline, which may be correct. */
              return -3;
            }
          from_start += match + 1 - input;
          input = match + 1;
        }
      /* Need more input for a newline or <|>. */
      return -2;
    }
  return -4;
}

/**
 * @brief "Synchronise" the \ref from_server buffer.
 *
 * Move any OTP in the \ref from_server buffer to the front of the buffer.
 *
 * @return 0 success, -1 \ref from_server is full.
 */
int
sync_buffer ()
{
  if (from_server_start > 0 && from_server_start == from_server_end)
    {
      from_server_start = from_server_end = 0;
      tracef ("   server start caught end\n");
    }
  else if (from_server_start == 0)
    {
      if (from_server_end == from_buffer_size)
        {
          // FIX if the buffer is entirely full here then exit
          //     (or will hang waiting for buffer to empty)
          //     this could happen if the server sends a field with length >= buffer length
          //         could realloc buffer
          //             which may eventually use all mem and bring down manager
          //                 would only bring down the process serving the client
          //                 may lead to out of mem in other processes?
          //                 could realloc to an upper limit within avail mem
          tracef ("   server buffer full\n");
          return -1;
        }
    }
  else
    {
      /* Move the remaining partial line to the front of the buffer.  This
       * ensures that there is space after the partial line into which
       * serve_omp can read the rest of the line. */
      char* start = from_server + from_server_start;
      from_server_end -= from_server_start;
      memmove (from_server, start, from_server_end);
      from_server_start = 0;
#if TRACE
      from_server[from_server_end] = '\0';
      //tracef ("   new from_server: %s\n", from_server);
      tracef ("   new from_server_start: %i\n", from_server_start);
      tracef ("   new from_server_end: %i\n", from_server_end);
#endif
    }
  return 0;
}

/**
 * @brief Process any lines available in \ref from_server.
 *
 * Update server information according to the input from the server.
 * This includes updating the server state with \ref set_server_state
 * and \ref set_server_init_state, and updating server records with functions
 * like \ref add_server_preference and \ref append_task_open_port.
 *
 * This function simply records input from the server.  Output to the server
 * or client is always done via \ref process_omp_client_input in reaction to
 * client requests.
 *
 * @return 0 success, 1 received server BYE, -1 error.
 */
int
process_otp_server_input ()
{
  char* match;
  char* messages = from_server + from_server_start;
  char* input;
  int from_start, from_end;
  //tracef ("   consider %.*s\n", from_server_end - from_server_start, messages);

  /* First, handle special server states where the input from the server
   * ends in something other than <|> (usually a newline). */

  switch (server_init_state)
    {
      case SERVER_INIT_SENT_VERSION:
        /* Read over any whitespace left by the previous session. */
        while (from_server_start < from_server_end
               && (messages[0] == ' ' || messages[0] == '\n'))
          from_server_start++, messages++;
        if (from_server_end - from_server_start < 12)
          {
            /* Need more input. */
            if (sync_buffer ()) return -1;
            return 0;
          }
        if (strncasecmp ("< OTP/1.0 >\n", messages, 12))
          {
            tracef ("   server fail: expected \"< OTP/1.0 >, got \"%.12s\"\n\"\n",
                    messages);
            return -1;
          }
        from_server_start += 12;
        messages += 12;
        set_server_init_state (SERVER_INIT_GOT_VERSION);
        /* Fall through to attempt next step. */
      case SERVER_INIT_GOT_VERSION:
        if (from_server_end - from_server_start < 7)
          {
            /* Need more input. */
            if (sync_buffer ()) return -1;
            return 0;
          }
        if (strncasecmp ("User : ", messages, 7))
          {
            tracef ("   server fail: expected \"User : \", got \"%7s\"\n",
                    messages);
            return -1;
          }
        from_server_start += 7;
        messages += 7;
        set_server_init_state (SERVER_INIT_GOT_USER);
        if (sync_buffer ()) return -1;
        return 0;
      case SERVER_INIT_GOT_USER:
        /* Input from server after "User : " and before user name sent. */
        return -1;
      case SERVER_INIT_SENT_USER:
        if (from_server_end - from_server_start < 11)
          {
            /* Need more input. */
            if (sync_buffer ()) return -1;
            return 0;
          }
        if (strncasecmp ("Password : ", messages, 11))
          {
            tracef ("   server fail: expected \"Password : \", got \"%11s\"\n",
                    messages);
            return -1;
          }
        from_server_start += 11;
        messages += 11;
        set_server_init_state (SERVER_INIT_GOT_PASSWORD);
        if (sync_buffer ()) return -1;
        return 0;
      case SERVER_INIT_GOT_PASSWORD:
        /* Input from server after "Password : " and before password sent. */
        return -1;
      case SERVER_INIT_CONNECT_INTR:
      case SERVER_INIT_CONNECTED:
        /* Input from server before version string sent. */
        return -1;
      case SERVER_INIT_DONE:
      case SERVER_INIT_TOP:
        if (server_state == SERVER_DONE)
          switch (parse_server_done (&messages))
            {
              case -1: return -1;
              case -2:
                /* Need more input. */
                if (sync_buffer ()) return -1;
                return 0;
            }
        else if (server_state == SERVER_PREFERENCE_VALUE)
          switch (parse_server_preference_value (&messages))
            {
              case -2:
                /* Need more input. */
                if (sync_buffer ()) return -1;
                return 0;
            }
        else if (server_state == SERVER_RULE)
          while (1)
            {
              switch (parse_server_rule (&messages))
                {
                  case  0: continue;     /* Read a rule. */
                  case -1: break;        /* At final <|>. */
                  case -2:
                    /* Need more input. */
                    if (sync_buffer ()) return -1;
                    return 0;
                }
              break;
            }
        else if (server_state == SERVER_SERVER)
          /* Look for any newline delimited server commands. */
          switch (parse_server_server (&messages))
            {
              case  0: break;        /* Found newline delimited command. */
              case -1: return -1;    /* Error. */
              case -2:
                /* Need more input. */
                if (sync_buffer ()) return -1;
                return 0;
              case -3: break;        /* Next <|> is before next \n. */
              case -4: break;        /* Failed to find \n, try for <|>. */
            }
        else if (server_state == SERVER_PLUGIN_DEPENDENCY_DEPENDENCY
                 && parse_server_plugin_dependency_dependency (&messages))
          {
            /* Need more input for a <|>. */
            if (sync_buffer ()) return -1;
            return 0;
          }
        break;
    } /* switch (server_init_state) */

  /* Parse and handle any fields ending in <|>. */

  input = messages;
  from_start = from_server_start;
  from_end = from_server_end;
  while (from_start < from_end
         && (match = memchr (input, '<', from_end - from_start)))
    {
      if (((int) (match - input) - from_start + 1) < from_end
          && (match[1] == '|')
          && (match[2] == '>'))
        {
          char* message;
          /* Found a full field, process the field. */
#if 1
          tracef ("   server messages: %.*s...\n",
                  from_server_end - from_server_start < 200
                  ? from_server_end - from_server_start
                  : 200,
                  messages);
#endif
          message = messages;
          *match = '\0';
          from_server_start += match + 3 - messages;
          from_start = from_server_start;
          messages = match + 3;
          input = messages;
          tracef ("   server message: %s\n", message);

          /* Strip leading and trailing whitespace. */
          char* field = strip_space (message, match);

          tracef ("   server old state %i\n", server_state);
          tracef ("   server field: %s\n", field);
          switch (server_state)
            {
              case SERVER_BYE:
                if (strncasecmp ("BYE", field, 3))
                  return -1;
                set_server_init_state (SERVER_INIT_TOP);
                set_server_state (SERVER_DONE);
                switch (parse_server_done (&messages))
                  {
                    case  0:
                      if (sync_buffer ()) return -1;
                      return 1;
                    case -1: return -1;
                    case -2:
                      /* Need more input. */
                      if (sync_buffer ()) return -1;
                      return 0;
                  }
                break;
              case SERVER_DEBUG_DESCRIPTION:
                {
                  if (current_message)
                    {
                      // FIX \n for newline in description
                      char* description = g_strdup (field);
                      set_message_description (current_message, description);
                    }
                  set_server_state (SERVER_DEBUG_OID);
                  break;
                }
              case SERVER_DEBUG_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  //if (current_server_task)  HOST_START
                  set_server_state (SERVER_DEBUG_NUMBER);
                  break;
                }
              case SERVER_DEBUG_NUMBER:
                {
                  assert (current_message == NULL);

                  // FIX field could be "general"
                  int number;
                  char *name = g_newa (char, strlen (field));
                  char *protocol = g_newa (char, strlen (field));

                  if (sscanf (field, "%s (%i/%[^)])",
                              name, &number, protocol)
                      != 3)
                    {
                      number = atoi (field);
                      protocol[0] = '\0';
                    }
                  tracef ("   server got debug port, number: %i, protocol: %s\n",
                          number, protocol);

                  current_message = make_message (number, protocol);

                  set_server_state (SERVER_DEBUG_DESCRIPTION);
                  break;
                }
              case SERVER_DEBUG_OID:
                {
                  if (current_message)
                    {
                      char* oid = g_strdup (field);
                      set_message_oid (current_message, oid);

                      append_debug_message (current_server_task, current_message);
                      current_message = NULL;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_HOLE_DESCRIPTION:
                {
                  if (current_message)
                    {
                      // FIX \n for newline in description
                      char* description = g_strdup (field);
                      set_message_description (current_message, description);
                    }
                  set_server_state (SERVER_HOLE_OID);
                  break;
                }
              case SERVER_HOLE_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  //if (current_server_task)  HOST_START
                  set_server_state (SERVER_HOLE_NUMBER);
                  break;
                }
              case SERVER_HOLE_NUMBER:
                {
                  assert (current_message == NULL);

                  // FIX field could be "general"
                  int number;
                  char *name = g_newa (char, strlen (field));
                  char *protocol = g_newa (char, strlen (field));

                  if (sscanf (field, "%s (%i/%[^)])",
                              name, &number, protocol)
                      != 3)
                    {
                      number = atoi (field);
                      protocol[0] = '\0';
                    }
                  tracef ("   server got hole port, number: %i, protocol: %s\n",
                          number, protocol);

                  current_message = make_message (number, protocol);

                  set_server_state (SERVER_HOLE_DESCRIPTION);
                  break;
                }
              case SERVER_HOLE_OID:
                {
                  if (current_message)
                    {
                      char* oid = g_strdup (field);
                      set_message_oid (current_message, oid);

                      append_hole_message (current_server_task, current_message);
                      current_message = NULL;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_INFO_DESCRIPTION:
                {
                  if (current_message)
                    {
                      // FIX \n for newline in description
                      char* description = g_strdup (field);
                      set_message_description (current_message, description);
                    }
                  set_server_state (SERVER_INFO_OID);
                  break;
                }
              case SERVER_INFO_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  //if (current_server_task)  HOST_START
                  set_server_state (SERVER_INFO_NUMBER);
                  break;
                }
              case SERVER_INFO_NUMBER:
                {
                  assert (current_message == NULL);

                  // FIX field could be "general"
                  int number;
                  char *name = g_newa (char, strlen (field));
                  char *protocol = g_newa (char, strlen (field));

                  if (sscanf (field, "%s (%i/%[^)])",
                              name, &number, protocol)
                      != 3)
                    {
                      number = atoi (field);
                      protocol[0] = '\0';
                    }
                  tracef ("   server got info port, number: %i, protocol: %s\n",
                          number, protocol);

                  current_message = make_message (number, protocol);

                  set_server_state (SERVER_INFO_DESCRIPTION);
                  break;
                }
              case SERVER_INFO_OID:
                {
                  if (current_message && current_server_task)
                    {
                      char* oid = g_strdup (field);
                      set_message_oid (current_message, oid);

                      append_info_message (current_server_task, current_message);
                      current_message = NULL;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_LOG_DESCRIPTION:
                {
                  if (current_message)
                    {
                      // FIX \n for newline in description
                      char* description = g_strdup (field);
                      set_message_description (current_message, description);
                    }
                  set_server_state (SERVER_LOG_OID);
                  break;
                }
              case SERVER_LOG_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  //if (current_server_task)  HOST_START
                  set_server_state (SERVER_LOG_NUMBER);
                  break;
                }
              case SERVER_LOG_NUMBER:
                {
                  assert (current_message == NULL);

                  // FIX field could be "general"
                  int number;
                  char *name = g_newa (char, strlen (field));
                  char *protocol = g_newa (char, strlen (field));

                  if (sscanf (field, "%s (%i/%[^)])",
                              name, &number, protocol)
                      != 3)
                    {
                      number = atoi (field);
                      protocol[0] = '\0';
                    }
                  tracef ("   server got log port, number: %i, protocol: %s\n",
                          number, protocol);

                  current_message = make_message (number, protocol);

                  set_server_state (SERVER_LOG_DESCRIPTION);
                  break;
                }
              case SERVER_LOG_OID:
                {
                  if (current_message && current_server_task)
                    {
                      char* oid = g_strdup (field);
                      set_message_oid (current_message, oid);

                      append_log_message (current_server_task, current_message);
                      current_message = NULL;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_NOTE_DESCRIPTION:
                {
                  if (current_message)
                    {
                      // FIX \n for newline in description
                      char* description = g_strdup (field);
                      set_message_description (current_message, description);
                    }
                  set_server_state (SERVER_NOTE_OID);
                  break;
                }
              case SERVER_NOTE_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  //if (current_server_task)  HOST_START
                  set_server_state (SERVER_NOTE_NUMBER);
                  break;
                }
              case SERVER_NOTE_NUMBER:
                {
                  assert (current_message == NULL);

                  // FIX field could be "general"
                  int number;
                  char *name = g_newa (char, strlen (field));
                  char *protocol = g_newa (char, strlen (field));

                  if (sscanf (field, "%s (%i/%[^)])",
                              name, &number, protocol)
                      != 3)
                    {
                      number = atoi (field);
                      protocol[0] = '\0';
                    }
                  tracef ("   server got note port, number: %i, protocol: %s\n",
                          number, protocol);

                  current_message = make_message (number, protocol);

                  set_server_state (SERVER_NOTE_DESCRIPTION);
                  break;
                }
              case SERVER_NOTE_OID:
                {
                  if (current_message && current_server_task)
                    {
                      char* oid = g_strdup (field);
                      set_message_oid (current_message, oid);

                      append_note_message (current_server_task, current_message);
                      current_message = NULL;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_PLUGIN_DEPENDENCY_NAME:
                {
                  if (strlen (field) == 0)
                    {
                      set_server_state (SERVER_DONE);
                      switch (parse_server_done (&messages))
                        {
                          case -1: return -1;
                          case -2:
                            /* Need more input. */
                            if (sync_buffer ()) return -1;
                            return 0;
                        }
                      break;
                    }
                  char* name = g_strdup (field);
                  make_current_server_plugin_dependency (name);
                  set_server_state (SERVER_PLUGIN_DEPENDENCY_DEPENDENCY);
                  if (parse_server_plugin_dependency_dependency (&messages))
                    {
                      /* Need more input for a <|>. */
                      if (sync_buffer ()) return -1;
                      return 0;
                    }
                  break;
                }
              case SERVER_PLUGIN_DEPENDENCY_DEPENDENCY:
                {
                  char* dep = g_strdup (field);
                  append_to_current_server_plugin_dependency (dep);
                  if (parse_server_plugin_dependency_dependency (&messages))
                    {
                      /* Need more input for a <|>. */
                      if (sync_buffer ()) return -1;
                      return 0;
                    }
                  break;
                }
              case SERVER_PLUGINS_MD5:
                {
                  char* md5 = g_strdup (field);
                  tracef ("   server got plugins_md5: %s\n", md5);
                  server.plugins_md5 = md5;
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_PORT_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  set_server_state (SERVER_PORT_NUMBER);
                  break;
                }
              case SERVER_PORT_NUMBER:
                {
                  if (current_server_task)
                    {
                      int number;
                      char *name = g_newa (char, strlen (field));
                      char *protocol = g_newa (char, strlen (field));

                      if (sscanf (field, "%s (%i/%[^)])",
                                  name, &number, protocol)
                          != 3)
                        {
                          number = atoi (field);
                          protocol[0] = '\0';
                        }
                      tracef ("   server got open port, number: %i, protocol: %s\n",
                              number, protocol);
                      append_task_open_port (current_server_task,
                                             number,
                                             protocol);
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_PREFERENCE_NAME:
                {
                  if (strlen (field) == 0)
                    {
                      set_server_state (SERVER_DONE);
                      switch (parse_server_done (&messages))
                        {
                          case -1: return -1;
                          case -2:
                            /* Need more input. */
                            if (sync_buffer ()) return -1;
                            return 0;
                        }
                      break;
                    }
                  char* name = g_strdup (field);
                  current_server_preference = name;
                  set_server_state (SERVER_PREFERENCE_VALUE);
                  switch (parse_server_preference_value (&messages))
                    {
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_RULE:
                /* A <|> following a rule. */
                set_server_state (SERVER_DONE);
                switch (parse_server_done (&messages))
                  {
                    case -1: return -1;
                    case -2:
                      /* Need more input. */
                      if (sync_buffer ()) return -1;
                      return 0;
                  }
                break;
              case SERVER_SERVER:
                if (strncasecmp ("BYE", field, 3) == 0)
                  set_server_state (SERVER_BYE);
                else if (strncasecmp ("DEBUG", field, 5) == 0)
                  set_server_state (SERVER_HOLE_HOST);
                else if (strncasecmp ("HOLE", field, 4) == 0)
                  set_server_state (SERVER_HOLE_HOST);
                else if (strncasecmp ("INFO", field, 4) == 0)
                  set_server_state (SERVER_INFO_HOST);
                else if (strncasecmp ("LOG", field, 3) == 0)
                  set_server_state (SERVER_LOG_HOST);
                else if (strncasecmp ("NOTE", field, 4) == 0)
                  set_server_state (SERVER_NOTE_HOST);
                else if (strncasecmp ("PLUGINS_MD5", field, 11) == 0)
                  set_server_state (SERVER_PLUGINS_MD5);
                else if (strncasecmp ("PORT", field, 4) == 0)
                  set_server_state (SERVER_PORT_HOST);
                else if (strncasecmp ("PREFERENCES", field, 11) == 0)
                  {
                    maybe_free_server_preferences ();
                    make_server_preferences ();
                    set_server_state (SERVER_PREFERENCE_NAME);
                  }
                else if (strncasecmp ("RULES", field, 5) == 0)
                  {
                    maybe_free_server_rules ();
                    make_server_rules ();
                    set_server_state (SERVER_RULE);
                    while (1)
                      {
                        switch (parse_server_rule (&messages))
                          {
                            case  0: continue;     /* Read a rule. */
                            case -1: break;        /* At final <|>. */
                            case -2:
                              /* Need more input. */
                              if (sync_buffer ()) return -1;
                              return 0;
                          }
                        break;
                      }
                    break;
                  }
                else if (strncasecmp ("TIME", field, 4) == 0)
                  {
                    set_server_state (SERVER_TIME);
                  }
                else if (strncasecmp ("STATUS", field, 6) == 0)
                  {
                    set_server_state (SERVER_STATUS_HOST);
                  }
                else
                  {
                    tracef ("New server command to implement: %s\n",
                            field);
                    return -1;
                  }
                break;
              case SERVER_STATUS_ATTACK_STATE:
                {
                  if (current_server_task)
                    {
                      char* state = g_strdup (field);
                      tracef ("   server got attack state: %s\n", state);
                      if (current_server_task->attack_state)
                        free (current_server_task->attack_state);
                      current_server_task->attack_state = state;
                    }
                  set_server_state (SERVER_STATUS_PORTS);
                  break;
                }
              case SERVER_STATUS_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  set_server_state (SERVER_STATUS_ATTACK_STATE);
                  break;
                }
              case SERVER_STATUS_PORTS:
                {
                  if (current_server_task)
                    {
                      unsigned int current, max;
                      tracef ("   server got ports: %s\n", field);
                      if (sscanf (field, "%u/%u", &current, &max) == 2)
                        set_task_ports (current_server_task, current, max);
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_TIME:
                {
                  if (strncasecmp ("HOST_START", field, 10) == 0)
                    set_server_state (SERVER_TIME_HOST_START_HOST);
                  else if (strncasecmp ("HOST_END", field, 8) == 0)
                    set_server_state (SERVER_TIME_HOST_END_HOST);
                  else if (strncasecmp ("SCAN_START", field, 10) == 0)
                    set_server_state (SERVER_TIME_SCAN_START);
                  else if (strncasecmp ("SCAN_END", field, 8) == 0)
                    set_server_state (SERVER_TIME_SCAN_END);
                  else
                    abort (); // FIX read all fields up to <|> SERVER?
                  break;
                }
              case SERVER_TIME_HOST_START_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  set_server_state (SERVER_TIME_HOST_START_TIME);
                  break;
                }
              case SERVER_TIME_HOST_START_TIME:
                {
                  if (current_server_task)
                    {
                      char* time = g_strdup (field);
                      tracef ("   server got start time: %s\n", time);
                      if (current_server_task->start_time)
                        free (current_server_task->start_time);
                      current_server_task->start_time = time;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_TIME_HOST_END_HOST:
                {
                  //if (strncasecmp ("chiles", field, 11) == 0) // FIX
                  set_server_state (SERVER_TIME_HOST_END_TIME);
                  break;
                }
              case SERVER_TIME_HOST_END_TIME:
                {
                  if (current_server_task)
                    {
                      char* time = g_strdup (field);
                      tracef ("   server got end time: %s\n", time);
                      if (current_server_task->end_time)
                        free (current_server_task->end_time);
                      current_server_task->end_time = time;

                      if (save_report (current_server_task)) return -1;
                    }
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_TIME_SCAN_START:
                {
                  if (current_server_task)
                    current_server_task->running = 2;
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_TIME_SCAN_END:
                {
                  if (current_server_task)
                    current_server_task->running = 3;
                  current_server_task = NULL;
                  set_server_state (SERVER_DONE);
                  switch (parse_server_done (&messages))
                    {
                      case -1: return -1;
                      case -2:
                        /* Need more input. */
                        if (sync_buffer ()) return -1;
                        return 0;
                    }
                  break;
                }
              case SERVER_TOP:
              default:
                tracef ("   switch t\n");
                tracef ("   cmp %i\n", strncasecmp ("SERVER", field, 6));
                if (strncasecmp ("SERVER", field, 6))
                  return -1;
                set_server_state (SERVER_SERVER);
                /* Look for any newline delimited server commands. */
                switch (parse_server_server (&messages))
                  {
                    case  0: break;        /* Found newline delimited command. */
                    case -1: return -1;    /* Error. */
                    case -2:
                      /* Need more input. */
                      if (sync_buffer ()) return -1;
                      return 0;
                    case -3: break;        /* Next <|> is before next \n. */
                    case -4: break;        /* Failed to find \n, try for <|>. */
                  }
                break;
            }

          tracef ("   server new state: %i\n", server_state);
        }
      else
        {
          from_start += match + 1 - input;
          input = match + 1;
        }
    }

  if (sync_buffer ()) return -1;
  return 0;
}
