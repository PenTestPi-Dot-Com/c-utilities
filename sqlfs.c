
/* 
 * @returns The data needed for each sales attempt.
 * 
 * Dear Human and a Sphinx script:
 * 
 * I am a debian only helper utility that makes reasources needed for every sale.
 * I use a running monero-walllet-rpc server to premake download directories, 
 * payment ids, new integrated monero addresses and 
 * in the future more stuff like OTP secrets, user names and passwords, etc.
 * I should come with my own cmake config and readme. Provided that you have a 
 * compiler, libsqlite3 and libjson-c you should be able to make me work me with:
 * cmake . [enter]
 * make    [enter]  
 * make run [enter]
 * 
 * However if you have gcc and you just want to try me quick and dirty you can do:
 * 
 * gcc sqlfs.c -ljson-c -lsqlite3 -o sfs && ./sfs
 *
 * Break a pinky. 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <sqlite3.h>

int isRpcWorking();
int getaddress(char*);
int makeNewDir(); 

char newMoneroAddress[1024];
char rpc_creds_file[1024];
char newDir[1024];
char newPaymentId[1024];

	
int main(int argc, char* argv[]) {
	
	int i = 1;
	sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("FutureTransactions.db", &db);
	char *sql;
	char tempstring[1024];
	char str[10];
	
	strcpy(argv[0], "");
	if (argc != 1) {
        fprintf(stderr, "%s\n", "I don't take any arguments. Try again by just running the " 
        "program.");
        exit(1);
    }
    
	if (isRpcWorking() != 0) {
		printf("%s\n", "Something went wrong, exiting.");
		exit(1);
	}

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

	sql = "DROP TABLE IF EXISTS UnusedPaymentData;" 
		"CREATE TABLE UnusedPaymentData(Id INT, Directory TEXT, PaymentId TEXT,"
		" MoneroAddress TEXT);"; 

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	
	if (rc != SQLITE_OK ) {
		printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return 1;
    }
    
	if (system("clear")) exit(1);
	
	for (i = 1; i < 200; i++){

		strcpy (rpc_creds_file, "/media/user/c2a5aa0e-7fd6-4bbf-8637-cc47ea80b855/"
					"monero-cli/monero-x86_64-linux-gnu-v0.17.3.0/monero-wallet-rpc.18083"
					".login");
	
				printf("\nWorking on customer info set number %d \n\n", i);
		getaddress(rpc_creds_file);
		makeNewDir();
				
   		strcpy(tempstring, "INSERT INTO UnusedPaymentData VALUES(");
		sprintf(str, "%d", i);
		strcat(tempstring, str);
		strcat(tempstring,", \'");
		strcat(tempstring, newDir);
		strcat(tempstring, "\', \'");
		strcat(tempstring, newPaymentId);
		strcat(tempstring, "\', \'");
		strcat(tempstring, newMoneroAddress);
		strcat(tempstring, "\');");

		sql = tempstring;

		rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	
		if (rc != SQLITE_OK ) {
			fprintf(stderr, "SQL error: %s\n", err_msg);
			sqlite3_free(err_msg);        
			sqlite3_close(db);
			return 1;
		}
	if (system("clear")) exit(1);
	}
	sqlite3_close(db);
	return (0);
}
		
int isRpcWorking(){
	
	FILE *fp;
	char buffer[1024];
	
	strcpy(buffer, "");
	fp = popen("pgrep monero-wallet-r", "r");
			if (fp == NULL) {
				printf("Failed to poll rpc pid. Your system might not have pgrep.");
				return(1);
			}
	fgets(buffer, sizeof(buffer), fp); 
	if (buffer[0] == '\0') {
		printf("Either you or your operating system renamed monero-wallet-rpc server or "
			"it is not running. Without it I can't do my thing. Sorry. ");
		return(2);
	}
	pclose(fp);
	return(0);
}


int getaddress(char *rpc_creds){

	FILE *fp;
	char paymentString[1024];
	char curlcommand[1024];
	char paymentId[17];
	int i;
	char buffer[1024];
	struct json_object *parsed_json;
	struct json_object *result;
	struct json_object *address;

	
	fp = fopen(rpc_creds, "r");
	fread(buffer, 1024, 1, fp);
	fclose(fp);

	
	fp = popen("openssl rand -hex 8", "r");
			if (fp == NULL) {
				printf("fail");
				exit(1);
			}
	fgets(paymentString, sizeof(paymentString), fp); 
	pclose(fp);

	for (i = 0; i < 16; i++){
		paymentId[i] = paymentString[i];
	}
	paymentId[16] = '\0';
	strcpy(newPaymentId, paymentId);
	
	strcpy (curlcommand, "curl --silent -u ");
	strcat (curlcommand, buffer);
	strcat (curlcommand, " --digest http://127.0.0.1:18083/json_rpc "
						 "-d '{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"\'ma"
						 "ke_integrated_address\'\",\"params\":\'\"{\\\"payment_i"
						 "d\\\":\\\"");
	strcat(curlcommand, paymentId);
	strcat(curlcommand, "\\\"}\"\'}\' -H \'Content-Type: application/json\' >"
						"/dev/shm/result.json");

	if (system(curlcommand)) exit(1); 


	strcpy (buffer, "");
	fp = fopen("/dev/shm/result.json", "r");
	fread(buffer, 1024, 1, fp);
	fclose(fp);

	remove("/dev/shm/result.json");
	parsed_json = json_tokener_parse(buffer);
	json_object_object_get_ex(parsed_json, "result", &result); 
	json_object_object_get_ex(result, "integrated_address", &address);
	strcpy (newMoneroAddress, json_object_get_string(address));

	return(0);
}

int makeNewDir(){
	
	FILE *fp;
	char buffer[1024];
	
	fp = popen("gpg --gen-random --armour 1 15 | base58", "r");
			if (fp == NULL) {
				printf("fail");
				exit(1);
			}
	fgets(buffer, sizeof(buffer), fp);
	strcpy(newDir, buffer);	
	pclose(fp);
	return 0;
}
	
	
