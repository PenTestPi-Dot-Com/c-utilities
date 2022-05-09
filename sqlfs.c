
/*
 * @returns The data needed for each sales attempt. 05/04/2022.
 *
 * Dear Human and a Sphinx script:
 *
 * I am an x86 linux only helper utility that makes reasources needed for every sale.
 * I use a running monero-walllet-rpc server to premake download directories,
 * payment ids, new integrated monero addresses and
 * in the future more stuff like OTP secrets, user names and passwords, etc.
 * I should come with my own cmake config and readme. Provided that you have a
 * compiler, libsqlite3 and libjson-c you should be able to make me work me with:
 * cmake . [enter]
 * make    [enter]
 * make run [enter]
 *
 * However if you have gcc and you just want to try me quick and dirty you can copy/paste:
 *
 * clear; gcc sqlfs.c -ljson-c -lsqlite3 -Wall -Wextra -Wpedantic -O3 -s -m64 -mrdrnd -o sfs && ./sfs
 *
 * Break a pinky.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include <time.h>
#include <immintrin.h>		// _rdrand64

int isRpcWorking ();
int getaddress (char *, char *);
int makeNewDir ();
int get_newPaymentId ();

char newMoneroAddress[1024];
char rpc_creds_file[1024];
char newDir[1024];
char newPaymentId[1024];

int
main (int argc, char *argv[])
{
  int i = 1;
  sqlite3 *db;
  char *err_msg = 0;

  int rc = sqlite3_open ("FutureTransactions.db", &db);
  char *sql;
  char tempstring[1024];
  char str[10];

  srand (time (NULL));		// for /dev/urandom. Remove if native x86 call is used.


  argv[0] = '\0';
  if (argc != 1)
    {
      fprintf (stderr, "%s\n",
	       "I don't take any arguments at this time. Try again by just running the "
	       "program.");
      exit (1);
    }

  if (isRpcWorking () != 0)
    {
      printf ("%s\n", "Something went wrong, exiting.");
      exit (1);
    }

  if (rc != SQLITE_OK)
    {
      fprintf (stderr, "Cannot open database: %s\n", sqlite3_errmsg (db));
      sqlite3_close (db);
      exit (1);
    }

  sql = "DROP TABLE IF EXISTS UnusedPaymentData;"
    "CREATE TABLE UnusedPaymentData(Id TEXT, Directory TEXT, PaymentId TEXT,"
    " MoneroAddress TEXT);";

  rc = sqlite3_exec (db, sql, 0, 0, &err_msg);

  if (rc != SQLITE_OK)
    {
      printf ("SQL error: %s\n", err_msg);
      sqlite3_free (err_msg);
      sqlite3_close (db);
      return 1;
    }

  for (i = 1; i < 10001; i++)
    {

      strcpy (rpc_creds_file,
	      "/media/user/c2a5aa0e-7fd6-4bbf-8637-cc47ea80b855/"
	      "monero-cli/monero-x86_64-linux-gnu-v0.17.3.0/monero-wallet-rpc.18083"
	      ".login");

      makeNewDir ();

      get_newPaymentId ();

      getaddress (rpc_creds_file, newPaymentId);

      strcpy (tempstring, "INSERT INTO UnusedPaymentData VALUES(\'");
      sprintf (str, "%08d", i);
      strcat (tempstring, str);
      strcat (tempstring, "\', \'");
      strcat (tempstring, newDir);
      strcat (tempstring, "\', \'");
      strcat (tempstring, newPaymentId);
      strcat (tempstring, "\', \'");
      strcat (tempstring, newMoneroAddress);
      strcat (tempstring, "\');");

      sql = tempstring;

      rc = sqlite3_exec (db, sql, 0, 0, &err_msg);

      if (rc != SQLITE_OK)
	{
	  fprintf (stderr, "SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (db);
	  return 1;
	}
    }
  sqlite3_close (db);
  return (0);
}

int
isRpcWorking ()
{
  FILE *fp;
  char buffer[1024];

  strcpy (buffer, "");
  fp = popen ("pgrep monero-wallet-r", "r");
  if (fp == NULL)
    {
      printf ("Failed to poll rpc pid. Your system might not have pgrep.");
      return (1);
    }
  if (!fgets (buffer, sizeof (buffer), fp))
    {

      if (ferror (fp))
	{			// case of error before read was completed
	  printf ("%s\n",
		  "I started looking for monero wallet but then something blew up. Quitting");
	  exit (1);
	}
      else
	{			// case of EOF before anything else was read -- empty file
	  printf ("%s\n",
		  "Either you or your operating system renamed monero-wallet-rpc server or "
		  "it is not running. Without it I can't do my thing. Sorry. ");
	  exit (1);
	}
    }
  fclose (fp);

  return (0);
}

int
get_newPaymentId ()
{

  unsigned long long result = 0ULL;
  int i, rc, leftOver;
  char choices[17] = "0123456789abcedf";
  char id[17];

  for (i = 0; i < 16; i++)
    {
      rc = _rdrand64_step (&result);
      if (!rc)
	{
	  puts ("rand failed, aren't we on x86?");
	  exit (1);
	}
      leftOver = result % 16;
      id[i] = choices[leftOver];
    }
  id[16] = '\0';
  strcpy (newPaymentId, id);
  return (0);
}

int
getaddress (char *rpc_creds, char *newPaymentId)
{

  FILE *fp;
  char curlcommand[1024];

  int i;
  char buffer[1024];
  struct json_object *parsed_json;
  struct json_object *result;
  struct json_object *address;
  char temp[1];

  fp = fopen (rpc_creds, "r");
  if (!fread (buffer, 31, 1, fp))
    exit (1);
  fclose (fp);
  buffer[31] = '\0';


  strcpy (curlcommand, "curl --silent -u ");
  strcat (curlcommand, buffer);
  strcat (curlcommand, " --digest http://127.0.0.1:18083/json_rpc "
	  "-d '{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"\'ma"
	  "ke_integrated_address\'\",\"params\":\'\"{\\\"payment_i"
	  "d\\\":\\\"");
  strcat (curlcommand, newPaymentId);
  strcat (curlcommand,
	  "\\\"}\"\'}\' -H \'Content-Type: application/json\' >"
	  " /dev/shm/result.json");

  if (system (curlcommand))
    exit (1);

  strcpy (buffer, "");

  fp = fopen ("/dev/shm/result.json", "r");
  if (fp == NULL)
    {
      printf ("Failed to open temp curl file.");
      exit (1);
    }

  i = 0;
  while (fread (temp, 1, 1, fp))
    {
      buffer[i] = temp[0];
      i++;
    }
  buffer[i] = '\0';
  fclose (fp);

  remove ("/dev/shm/result.json");
  parsed_json = json_tokener_parse (buffer);
  json_object_object_get_ex (parsed_json, "result", &result);
  json_object_object_get_ex (result, "integrated_address", &address);
  strcpy (newMoneroAddress, json_object_get_string (address));

  return (0);
}

char *
randstring (int length)
{
  static char charset[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ123456789";

  char *randomString = malloc (length + 1);

  if (length == 0)
    {
      puts ("Creating zero random characters is pointless");
      exit (1);
    }

  if (randomString)
    {
      for (int n = 0; n < length; n++)
	{
	  int key = rand () % (int) (sizeof (charset) - 1);
	  randomString[n] = charset[key];
	}
      randomString[length] = '\0';
    }
  return randomString;
}

int
makeNewDir ()
{
  char buffer[1024];
  char *randomness = randstring (32);

  strcpy (buffer, randomness);
  strcpy (newDir, buffer);
  free (randomness);
  return 0;
}
