
/*
 * @returns The data needed for each sales attempt. 05/11/2022.
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
 * P.S. There's more narative at the bottom in case you are into long, drawn-out pointless prose trying to explain
 * poor coding practices and dubious structural decisions.
 * 
 *  
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include <time.h>
#include <immintrin.h>
#include <curl/curl.h>
#include <ctype.h>

int isRpcWorking();
int getaddress(char *, char *);
int makeNewDir();
int get_newPaymentId();
int fetchIt(char[]);

char curlItResult[1024];
char newMoneroAddress[1024];
char rpc_creds_file[1024];
char newDir[1024];
char newPaymentId[1024];

struct string {
  char *ptr;
  size_t len;
};

int main(int argc, char *argv[])
{
	FILE *fp;
	unsigned short i;
	sqlite3 *db;
	char *err_msg = 0;
	int rc = sqlite3_open("FutureTransactions.db", &db);
	char *sql;
	char tempstring[1024];
	char str[10];
	char buffer[1024];
	struct json_object *parsed_json;
	struct json_object *result;
	struct json_object *address;
	char temp[1];
	int numberOfAddresses = 10000;

	srand(time(NULL));

	if (argc > 2) {
		fprintf(stderr, "%s\n","I only take one number of new addresses as an argment for now. Try again.");
		exit(1);
	}

	if (argc == 2) {
		strcpy(tempstring, argv[1]);
		i = 0;
		while (tempstring[i] != '\0') {
			if (!isdigit(tempstring[i])) {
				puts("Argument has to be a number.");
				exit(1);
			}
			i++;
		}
	
		if ((strlen(tempstring)) > 5) {
			puts("You are asking for too much. Pick a lower number.");
			exit(1);
		}
		numberOfAddresses = (atoi(argv[1]));
		
	}
	strcpy(tempstring, "");

	if (isRpcWorking() != 0) {
		printf("%s\n", "Something went wrong, exiting.");
		exit(1);
	}

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n",
			sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sql = "DROP TABLE IF EXISTS UnusedPaymentData;"
	    "CREATE TABLE UnusedPaymentData(Id TEXT, Directory TEXT, PaymentId TEXT,"
	    " MoneroAddress TEXT);";

	rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if (rc != SQLITE_OK) {
		printf("SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return 1;
	}

	strcpy(rpc_creds_file,
		       "/media/user/c2a5aa0e-7fd6-4bbf-8637-cc47ea80b855/"
		       "monero-cli/monero-x86_64-linux-gnu-v0.17.3.0/monero-wallet-rpc.18083"
		       ".login");
	strcat(rpc_creds_file, "0");


	for (i = 1; i < (numberOfAddresses + 1); i++) {

		makeNewDir();

		get_newPaymentId();

		fp = fopen(rpc_creds_file, "r");
		if (!fread(buffer, 31, 1, fp))
			exit(1);
		fclose(fp);
		buffer[31] = '\0';

		fetchIt(buffer);

		parsed_json = json_tokener_parse(curlItResult);
		json_object_object_get_ex(parsed_json, "result", &result);
		json_object_object_get_ex(result, "integrated_address", &address);
		strcpy(newMoneroAddress, json_object_get_string(address));

		strcpy(tempstring, "INSERT INTO UnusedPaymentData VALUES(\'");
		sprintf(str, "%08d", i);
		strcat(tempstring, str);
		strcat(tempstring, "\', \'");
		strcat(tempstring, newDir);
		strcat(tempstring, "\', \'");
		strcat(tempstring, newPaymentId);
		strcat(tempstring, "\', \'");
		strcat(tempstring, newMoneroAddress);
		strcat(tempstring, "\');");

		sql = tempstring;

		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", err_msg);
			sqlite3_free(err_msg);
			sqlite3_close(db);
			return 1;
		}
	}
	sqlite3_close(db);
	return (0);
}

int isRpcWorking()
{
	FILE *fp;
	char buffer[1024];

	strcpy(buffer, "");
	fp = popen("pgrep monero-wallet-r", "r");
	if (fp == NULL) {
		printf
		    ("Failed to poll rpc pid. Your system might not have pgrep.");
		return (1);
	}
	if (!fgets(buffer, sizeof(buffer), fp)) {

		if (ferror(fp)) {	// case of error before read was completed
			printf("%s\n",
			       "I started looking for monero wallet but then something blew up. Quitting");
			exit(1);
		} else {	// case of EOF before anything else was read -- empty file
			printf("%s\n",
			       "Either you or your operating system renamed monero-wallet-rpc server or "
			       "it is not running. Without it I can't do my thing. Sorry. ");
			exit(1);
		}
	}
	fclose(fp);

	return (0);
}

int get_newPaymentId()
{
	unsigned long long result = 0ULL;
	int i, rc, leftOver;
	char choices[17] = "0123456789abcedf";
	char id[17];

	for (i = 0; i < 16; i++) {
		rc = _rdrand64_step(&result);
		if (!rc) {
			puts("rand failed, aren't we on x86?");
			exit(1);
		}
		leftOver = result % 16;
		id[i] = choices[leftOver];
	}
	id[16] = '\0';
	strcpy(newPaymentId, id);
	return (0);
}

char *randstring(int length)
{
	static char charset[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ123456789";

	char *randomString = malloc(length + 1);

	if (length == 0) {
		puts("Creating zero random characters is pointless");
		exit(1);
	}

	if (randomString) {
		for (int n = 0; n < length; n++) {
			int key = rand() % (int)(sizeof(charset) - 1);
			randomString[n] = charset[key];
		}
		randomString[length] = '\0';
	}
	return randomString;
}

int makeNewDir()
{
	char buffer[1024];
	char *randomness = randstring(32);

	strcpy(buffer, randomness);
	strcpy(newDir, buffer);
	free(randomness);
	return 0;
}

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

int fetchIt(char rpc_creds[])
{
  CURL *curl;
  CURLcode res;
  struct curl_slist *headers=NULL;
 
  char postthis[1024] = "{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"make_integrated_address\""
								",\"params\":"
								"{\"payment_id\":\"";
	strcat(postthis, newPaymentId);
	strcat(postthis, "\"}}");

	curl = curl_easy_init();
	if(curl) {

		struct string s;
		init_string(&s);
	
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "Accept: */*");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:18083/json_rpc");
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
		curl_easy_setopt(curl, CURLOPT_USERPWD, rpc_creds);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
 	    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

		res = curl_easy_perform(curl);

		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
 
		strcpy(curlItResult, s.ptr);
		curlItResult[strlen(s.ptr) + 1] = 0;


		free(s.ptr);

		curl_easy_cleanup(curl);
	}
	return 0;
}
