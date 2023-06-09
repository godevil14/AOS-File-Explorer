
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <bits/stdc++.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

using namespace std;


#define int long long

typedef std::string string;
// template<typename T>
// typedef std::vector<T> vector<T>;


#include "debug.cpp"

string delimiter=";";

//basically the python split function
vector<string> split(string s, char delim){
	vector<string> ret;
	string cur;
	for(int i=0; i<s.size(); i++){
		if(s[i] == delim){
			if(cur.size() != 0)
				ret.push_back(cur);
			cur = "";
		}
		else{
			cur += s[i];
		}
	}
	if(cur.size() != 0){
		ret.push_back(cur);
	}
	return ret;
}

//convert the vector of string to a single string for a path
string vectorToPath(vector<string> &v){
	string ret;
	for(int i = 0; i<v.size(); i++){
		ret += '/';
		ret += v[i];
	}
	return ret+'/';
}

string getAbsolutePath(string inp, vector<string> curLocation, bool forceDirectory){
	// if forceDirectory is 1, the path must belong to a directory
	print(curLocation);
	debug(inp);
	string folder = inp;
  	vector<string> vec = split(folder, '/');
	vector<string> resultant;
  	int beginIndex = 0;

  	if(folder[0] == '/'){
		beginIndex = 0;
	}
	else if(vec[0] == "~"){
		struct passwd *pw = getpwuid(getuid());
		resultant = split((pw->pw_dir), '/');
		beginIndex = 1;
	}
	else{
		resultant = curLocation;
		beginIndex = 0;
		
	}

	//check after each each push/pop if the directory/file entered is correct
	for(int i=beginIndex; i<vec.size(); i++){
		if(vec[i] != ".." && vec[i] != ".")\
			resultant.push_back(vec[i]);
		else if(vec[i] == "."){
			/*do nothing*/}
		else if(resultant.size() != 0 && vec[i] == "..")
			resultant.pop_back();

		struct stat buffer;
		if(forceDirectory){
			if(stat(vectorToPath(resultant).c_str(), &buffer) == -1) // directory does not exist
				return "";
			else if(!S_ISDIR(buffer.st_mode)) // trying to go inside a file
				return "";
		}
		else{
			if(i == vec.size()-1){
				string s = vectorToPath(resultant);
				s.pop_back();
				if(stat(s.c_str(), &buffer) == -1) // file does not exist
					return "";
			}
			else{
				if(stat(vectorToPath(resultant).c_str(), &buffer) == -1) // directory does not exist
					return "";
				else if(!S_ISDIR(buffer.st_mode)) // trying to go inside a file
					return "";
			}
		}
	
	}

	if(forceDirectory)
		return vectorToPath(resultant);
	else{
		string s = vectorToPath(resultant);
		s.pop_back();
		return s;
	}
}

int tracker_port = 8080;

void upload_to_peer_thread(int new_socket, string username){
	debug("SENDING");
	char buffer[1024];
	read(new_socket, buffer, 1024);
	send(new_socket, "1", 1024, 0);
	vector<string> spl = split(buffer, ';');
	int number_of_chunks = stoi(spl[0]);
	string filename = spl[1];
	int chunksize = 1024*512;
	int chunksubsize = 1024*16;

	char tmp[1024];
    getcwd(tmp, 1024);
    string destpath;
    ifstream ifile;
    int uploads = 0;
    // debug("OEIFE");
    if(getAbsolutePath(username+"/uploads/"+filename, split(tmp, '/'), 0) != ""){
    	ifile.open(username+"/uploads/"+filename, ios::in|ios::binary|ios::ate); //sender is a seeder
    	uploads = 1;
    }
    else if(getAbsolutePath(username+"/downloads/"+filename, split(tmp, '/'), 0) != ""){
    	ifile.open(username+"/downloads/"+filename, ios::in|ios::binary|ios::ate); //sender is a leecher
    	uploads = 0;
    }
    else{
    	ifile.open(username+"/download_complete/"+filename, ios::in|ios::binary|ios::ate);
    }
    // debug("HERE");



	// ifstream ifile(username+"/uploads/"+filename, ios::in|ios::binary|ios::ate);
	int size = ifile.tellg();
	for(int i=0; i<number_of_chunks; i++){
		int lol;
		// debug(i, new_socket);
		char buffer[1024];
		lol = read(new_socket, buffer, 1024);
		if(lol <= 0) break;
		int chunk_number = stoi(buffer);
		char* data = (char *)malloc(sizeof(char)*chunksize);
		char* shablock = new char [chunksize];

		int start = chunk_number*chunksize;
		int end = min(start+chunksize, size);
		int newchunksize = end-start;

		// debug(i, newchunksize, start, end, size, chunk_number);

		char waste[1024];
		// debug(newchunksize);
		send(new_socket, to_string(newchunksize).c_str(), 1024, 0);
		lol = read(new_socket, waste, 1024);
		if(lol <= 0) break;
		for(int i=0; i<(newchunksize/chunksubsize) + (newchunksize%chunksubsize != 0); i++){
			char ack[1024];
			int start = chunk_number*chunksize+i*chunksubsize;
			int end = min(start+chunksubsize, size);
			int newchunksize = end-start;
			ifile.seekg(start);
			ifile.read(data, newchunksize);
			for(int j=0; j<newchunksize; j++)
				shablock[i*chunksubsize+j] = data[j];
			send(new_socket, to_string(newchunksize).c_str(), 1024, 0);
			lol = read(new_socket, ack, 1024);
			if(lol <= 0) break;
			send(new_socket, data, chunksubsize+10, 0);
			lol = read(new_socket, ack, 1024);
			if(lol <= 0) break;
		}
		unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
		SHA1((unsigned char*)shablock, sizeof(shablock) - 1, hash);
		send(new_socket, hash, 1024, 0);

		free(data);
		free(shablock);

	}
	ifile.close();
}

string USERNAME;

void download_from_peer_thread(string download_ip, int download_port, vector<int> chunk_numbers, string filename, vector<string> shas, string destpath, int file_size, int tracker_port, string groupid){
		// debug("JJ");
	struct sockaddr_in serv_addr;
	int sock = 0, valread, client_fd;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

    // debug("TYUI");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(download_port);
	// debug(download_port, download_ip);
	// print(chunk_numbers);

	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return;
	}

	if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
		printf("\nConnection Failed \n");
		return;
	}

	// debug("ASDFG");



	struct sockaddr_in serv_addr2;
	int sock2 = 0, valread2, client_fd2;

    if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return;
    }

	serv_addr2.sin_family = AF_INET;
	serv_addr2.sin_port = htons(tracker_port);
	// debug(download_port, download_ip);
	// print(chunk_numbers);

	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr2.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return;
	}

	if ((client_fd2 = connect(sock2, (struct sockaddr*)&serv_addr2, sizeof(serv_addr2))) < 0) {
		printf("\nConnection Failed \n");
		return;
	}
	// debug("QWERTY");

	string filemetadata = filename + delimiter + groupid + delimiter + USERNAME + delimiter + to_string(chunk_numbers.size()) + delimiter;
	send(sock2, filemetadata.c_str(), 1024, 0);
	char ack[1024];
	read(sock2, ack, 1024);

	int chunksubsize = 1024*16;
	int chunksize = 1024*512;
	char buffer[1024];
	ofstream ofile(USERNAME+"/downloads/"+filename, ios::binary);
	ofstream ofile2(destpath + "/" + filename, ios::binary);
	send(sock, (to_string(chunk_numbers.size()) + ";" + filename).c_str(), 1024, 0);
	read(sock, buffer, 1024);
	for(int i=0; i<chunk_numbers.size(); i++){
		// cout << "Taking " << chunk_numbers[i] << " from " << download_port << "\n";
		char buffer[1024];
		send(sock, to_string(chunk_numbers[i]).c_str(), 1024, 0);
		char chunkwritesizechar[1024];
		int lol = read(sock, chunkwritesizechar, 1024);
		if(lol <= 0){
			cout << "The file " << filename << " was corrupted\n";
			break;
		}


		int chunkwritesize = stoi(chunkwritesizechar);
		char* shablock = new char [chunksize];
		send(sock, "1", 1024, 0);

		// debug(i, ofile2.is_open());
		for(int j=0; j<chunkwritesize/chunksubsize + (chunkwritesize%chunksubsize != 0); j++){
			// debug(j);
			char data[chunksubsize+10], writesize[1024];
			lol = read(sock, writesize, 1024);
			// debug("CC");
			if(lol <= 0){
				cout << "The file " << filename << " was corrupted\n";
				break;
			}
			// debug(lol);
			send(sock, "1", 1024, 0);
			// debug(lol);
			lol = read(sock, data, chunksubsize+10);
			if(lol <= 0){
				cout << "The file " << filename << " was corrupted\n";
				break;
			}
			ofile.seekp(chunk_numbers[i]*chunksize + j*chunksubsize);
			ofile.write(data, stoi(writesize));
			// debug("DD");
			ofile2.seekp(chunk_numbers[i]*chunksize + j*chunksubsize);
			ofile2.write(data, stoi(writesize));
			for(int k=0; k<stoll(writesize); k++)
				shablock[j*chunksubsize+k] = data[k];
			send(sock, "1", 1024, 0);
		}
		// debug("AA");

		unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
		SHA1((unsigned char*)shablock, sizeof(shablock) - 1, hash);

		unsigned char expectedSHA[1024];
		lol = read(sock, expectedSHA, 1024);
		if(lol <= 0){
			cout << "The file " << filename << " was corrupted\n";
			break;
		}

		for(int i=0; i<20; i++){
			if(hash[i] != expectedSHA[i]){
				cout << "FILE CORRUPTED\n";
				break;
			}
		}

		// debug("BB");

		char download_complete[1024];
		send(sock2, to_string(chunk_numbers[i]).c_str(), 1024, 0);
		lol = read(sock2, download_complete, 1024);
		if(lol <= 0){
			cout << "The file " << filename << " was corrupted\n";
			break;
		}
		if(stoi(download_complete) == 1){
			rename((USERNAME+"/downloads/"+filename).c_str(), (USERNAME+"/download_complete/"+filename).c_str());
			cout << "DOWNLOAD OF " << filename << " COMPLETE!!\n";
		}
		free(shablock);
		// sleep(1);
	}
	ofile.close();
}

void listen_for_peer(string IP, int PORT){
	while(USERNAME == ""){
		int a = 1;
	}

	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	debug(IP, PORT);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(IP.c_str());
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	cout << "RUNNING ON IP:" << IP << " PORT:" << PORT << "\n";
	vector<thread> peers;
	while(true){
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// debug("XX");
		peers.push_back(thread(upload_to_peer_thread, new_socket, USERNAME));
	}
}

int32_t main(int32_t argc, char const* argv[])
{
	int sock = 0, valread, client_fd;
	struct sockaddr_in serv_addr;
	char buffer[1024] = { 0 };
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	if(argc != 3){
		cout << "INVALID NUMBER OF ARGUMENTS\n";
		return 0;
	}

	int tracker_number = 1;
	// srand(time(NULL));
	// tracker_number = rand()%2 + 1;

	// debug(argc);
	int tracker_port;
	string tracker_ip;
	string tracker_file = argv[2];
	int TRACKER_UPDATE_DOWNLOAD_PORT;
	ifstream tracker_info(tracker_file);
	tracker_info >> tracker_ip >> tracker_port >> TRACKER_UPDATE_DOWNLOAD_PORT;
	if(tracker_number == 2)
		tracker_info >> tracker_ip >> tracker_port >> TRACKER_UPDATE_DOWNLOAD_PORT;
	string portdetails = argv[1];
	vector<string> ipport = split(portdetails, ':');

	string IP = ipport[0];
	int PORT = stoi(ipport[1]);

	debug(tracker_ip, tracker_port, TRACKER_UPDATE_DOWNLOAD_PORT);

	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(tracker_port);


	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	// debug(client_fd);
	// send(sock, hello, strlen(hello), 0);
	// printf("Hello message sent\n");
	// valread = read(sock, buffer, 1024);
	// printf("%s\n", buffer);

	thread lol(listen_for_peer, IP, PORT);
	lol.detach();

	while(true){
		string s;
		// debug(sock);
		cin >> s;
		if(s == "create_user"){
			string uname, pass;
			cin >> uname >> pass;
			string buffer = s+delimiter+uname+delimiter+pass+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);

			if(successbuffer[0] == '0'){
				cout << "USER ALREADY EXISTS\n";
			}
			else{
				cout << "USER CREATED SUCCESSFULLY\n";
			}
		}
		else if(s == "login"){
			string uname, pass;
			cin >> uname >> pass;
			string buffer = s+delimiter+uname+delimiter+pass+delimiter+IP+delimiter+to_string(PORT)+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);

			// debug(successbuffer);
			if(successbuffer[0] == '0'){
				cout << "INVALID CREDENTIALS\n";
			}
			else if(successbuffer[0] == '2'){
				cout << "THIS USER HAS ALREADY LOGGED IN\n";
			}
			else{
				struct stat info;
				if(stat(uname.c_str(), &info )!= 0){
					mkdir(uname.c_str(), 0777);
					mkdir((uname+"/uploads").c_str(), 0777);
					mkdir((uname+"/downloads").c_str(), 0777);
					mkdir((uname+"/download_complete").c_str(), 0777);
				}
				USERNAME = uname;
				cout << "LOGGED IN SUCCESSFULLY\n";
			}
		}
		else if(s == "create_group"){
			string groupid;
			cin >> groupid;
			string buffer = s+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);

			// debug(successbuffer[0]);
			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID ALREADY EXISTS\n";
			}
			else{
				cout << "GROUP CREATED SUCCESSFULLY\n";
			}
		}

		else if(s == "join_group"){
			string groupid;
			cin >> groupid;
			string buffer = s+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID DOES NOT EXISTS\n";
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ALREADY HAVE A PENDING REQUEST IN THIS GROUP\n";
			}
			else if(successbuffer[0] == '3'){
				cout << "REQUEST SENT SUCCESSFULLY\n";
			}
			else{
				cout << "YOU ALREADY BELONG TO THE GROUP\n";
			}
		}

		else if(s == "leave_group"){
			string groupid;
			cin >> groupid;
			string buffer = s+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID DOES NOT EXISTS\n";
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU DO NOT BELONG TO THIS GROUP\n";
			}
			else{
				cout << "YOU HAVE LEFT THE GROUP\n";
			}
		}

		else if(s == "list_requests"){
			string groupid;
			cin >> groupid;
			string buffer = s+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);
			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
				send(sock, "0", 1024, 0);
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID DOES NOT EXISTS\n";
				send(sock, "0", 1024, 0);
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ARE NOT THE OWNER OF THAT GROUP\n";
				send(sock, "0", 1024, 0);
			}
			else{
				send(sock, "1", 1024, 0);
				char successbuffer[1024];
				int success = read(sock, successbuffer, 1024);
				cout << "PENDING REQUESTS FROM\n";
				vector<string> unames = split(successbuffer, ';');
				for(auto x: unames){
					cout << x << "\n";
				}
			}
		}

		else if(s == "accept_request"){
			string groupid, userid;
			cin >> groupid >> userid;
			string buffer = s+delimiter+groupid+delimiter+userid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID DOES NOT EXISTS\n";
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ARE NOT THE OWNER OF THAT GROUP\n";
			}
			else if(successbuffer[0] == '3'){
				cout << "THERE IS NO REQUEST FROM THAT USER\n";
			}
			else{
				cout << "USER SUCCESSFULLY ADDED TO GROUP\n";
			}
		}

		else if(s == "list_groups"){
			string buffer = s+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);
			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
				send(sock, "0", 1024, 0);
			}
			else{
				send(sock, "1", 1024, 0);
				char successbuffer2[1024];
				int success = read(sock, successbuffer2, 1024);
				// debug(successbuffer2);
				cout << "AVAILABLE GROUPS\n";
				vector<string> groupids = split(successbuffer2, ';');
				for(auto x: groupids){
					cout << x << "\n";
				}
			}
		}

		else if(s == "list_files"){
			string groupid;
			cin >> groupid;
			string buffer = s+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024] = {0};
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
				send(sock, "0", 1024, 0);
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP WITH THAT ID DOES NOT EXISTS\n";
				send(sock, "0", 1024, 0);
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ARE NOT IN THAT GROUP\n";
				send(sock, "0", 1024, 0);
			}
			else if(successbuffer[0] == '3'){
				send(sock, "1", 1024, 0);
				char successbuffer[1024];
				int success = read(sock, successbuffer, 1024);
				cout << "AVAILABLE FILES\n";
				vector<string> unames = split(successbuffer, ';');
				for(auto x: unames){
					cout << x << "\n";
				}
			}
			else{
				cout << "CONNECTION ERROR\n";
			}
		}

		else if(s == "upload_file"){
			string file_path, groupid, file_path_inp;
			cin >> file_path_inp >> groupid;


			char tmp[1024];
		    getcwd(tmp, 1024);
		    // debug(tmp);
		    file_path = getAbsolutePath(file_path_inp, split(tmp, '/'), 0);


			string filename = file_path;
			ifstream ifile(filename, ios::in|ios::binary|ios::ate);
			if(!ifile.is_open()){
				cout << "FILE DOES NOT EXIST\n";
				continue;
			}

			string file_name = split(file_path, '/').back();

			string buffer = s+delimiter+file_name+delimiter+groupid+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);
			char success[1024];
			read(sock, success, 1024);
			// debug(success[0]);
			if(success[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
				continue;
			}
			else if(success[0] == '1'){
				cout << "GROUP DOES NOT EXIST\n";
				continue;
			}
			else if(success[0] == '2'){
				cout << "YOU ARE NOT A MEMBER OF THAT GROUP\n";
				continue;
			}
			else if(success[0] == '3'){
				cout << "FILE ALREADY EXISTS IN THAT GROUP\n";
				continue;
			}

			std::ifstream  src(file_path, std::ios::binary);
		    std::ofstream  dst(USERNAME+"/uploads/"+file_name, std::ios::binary);

		    dst << src.rdbuf();

		    debug(file_path, USERNAME+"/uploads/"+file_name);

			streampos size;
			char * memblock;

			size = ifile.tellg();
		    ifile.seekg (0, ios::beg);

		    int chunksize = 1024*512;
		    int chunks = size/chunksize + (size%chunksize != 0);
		    char waste[1024];
		    send(sock, to_string(chunks).c_str(), 1024, 0);
		    read(sock, waste, 1024);
		    string fullhash;
		    for(int i=0; i<chunks-1; i++){
		    	char successbuffer[1024];
		    	char successbuffer2[1024];
		    	memblock = new char [chunksize];
			    ifile.read(memblock, chunksize);
			    unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
				SHA1((unsigned char*)memblock, sizeof(memblock) - 1, hash);
		    	std::string temps( reinterpret_cast< char const* >(hash) ) ;
		    	fullhash += temps;
		    	send(sock, hash, 1024, 0);
		    	read(sock, successbuffer2, 1024);
		    	delete [] memblock;
		    	// debug(hash);
		    }
		    char successbuffer[1024];
	    	char successbuffer2[1024];
		    memblock = new char [size-(chunks-1)*chunksize];
		    ifile.read(memblock, size-(chunks-1)*chunksize);
		    unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
			SHA1((unsigned char*)memblock, sizeof(memblock) - 1, hash);
			std::string temps( reinterpret_cast< char const* >(hash) ) ;
	    	fullhash += temps;
		    send(sock, hash, 1024, 0);
		    read(sock, successbuffer2, 1024);
		    // debug(hash);
		    delete [] memblock;

		    send(sock, to_string(size).c_str(), 1024, 0);
		    read(sock, successbuffer2, 1024);

		    cout << "UPLOAD OF " << filename << " COMPLETE!!!\n";

		    // debug(fullhash);
		    // int fullshachunklen = 512;
		    // send(sock, to_string(fullhash.size()/fullshachunklen + (fullhash.size()%fullshachunklen != 0)).c_str(), 1024, 0);
		    // for(int i=0; i<fullhash.size()/fullshachunklen; i++){
		    // 	char successbuffer[1024];
		    // 	char successbuffer2[1024];
		    // 	string temps;
		    // 	for(int j=fullshachunklen*i; j<fullshachunklen*(i+1); j++){
		    // 		temps += fullhash[j];
		    // 	}
		    // 	debug(temps);
		    // 	// send(sock, to_string(1024).c_str(), 1024, 0);
		    // 	// read(sock, successbuffer, 1024);
		    // 	send(sock, temps.c_str(), 1024, 0);
		    // 	read(sock, successbuffer2, 1024);
		    // }

		    // int remaining = fullhash.size()%fullshachunklen;
		    // // debug(fullhash);
		    // if(remaining != 0){
		    // 	char successbuffer[1024];
		    // 	char successbuffer2[1024];
		    // 	string temps;
		    // 	for(int j=fullhash.size()/fullshachunklen * fullshachunklen; j<fullhash.size(); j++){
		    // 		temps += fullhash[j];
		    // 	}
		    // 	debug(temps);
		    // 	// send(sock, to_string(remaining).c_str(), 1024, 0);
		    // 	// read(sock, successbuffer, 1024);
		    // 	send(sock, temps.c_str(), 1024, 0);
		    // 	read(sock, successbuffer2, 1024);
		    // }



		}

		else if(s == "download_file"){
			string groupid, filename, destpathinp;
			cin >> groupid >> filename >> destpathinp;
			char tmp[1024];
		    getcwd(tmp, 1024);
		    // debug(tmp);
		    string destpath;
		    destpath = getAbsolutePath(destpathinp, split(tmp, '/'), 1);
		    // debug(destpath);
		    if(destpath == ""){
		    	cout << "DESTINATION MUST BE A VALID DIRECTORY (NOT FILE)" << endl;
		    	continue;
		    }

		    debug(destpath);

			string buffer = s+delimiter+groupid+delimiter+filename+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
				continue;
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP DOES NOT EXIST\n";
				continue;
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ARE NOT A MEMBER OF THAT GROUP\n";
				continue;
			}
			else if(successbuffer[0] == '3'){
				cout << "FILE DOES NOT EXIST IN THAT GROUP\n";
				continue;
			}


			send(sock, "1", 1024, 0);

			// char file_size[1024];
			// read(sock, file_size, 1024);
			// send(sock, "1", 1024, 0);

			char buffer2[1024];
			read(sock, buffer2, 1024);
			send(sock, "1", 1024, 0);
			int lim = stoi(buffer2);
			// debug(lim);
			// debug("HHHHHHHHHHHHHHHHH");

			// vector<int> thread_resources(lim);
			// debug(lim);
			for(int i=0; i<lim; i++){
				// debug(i);
				char buffer[1024];
				read(sock, buffer, 1024);
				vector<string> spl = split(buffer, ';');
				// print(spl);
				int number_of_chunks = stoi(spl[2]);
				send(sock, "1", 1024, 0);
				vector<int> chunk_numbers;
				vector<string> shas;
				// debug("LL");
				for(int j=0; j<number_of_chunks; j++){
					// debug(j);
					char buffer[1024];
					read(sock, buffer, 1024);
					send(sock, "1", 1024, 0);
					chunk_numbers.push_back(stoi(buffer));
				}
				// for(int j=0; j<number_of_chunks; j++){
				// 	// debug(j);
				// 	char buffer[1024];
				// 	read(sock, buffer, 1024);
				// 	send(sock, "1", 1024, 0);
				// 	shas.push_back(buffer);
				// }
				// print(spl);
				// debug(chunk_numbers.size());

				// thread_resources[i] = number_of_chunks;
				// thread tracker_comm();
				thread thr(download_from_peer_thread, spl[0], stoi(spl[1]), chunk_numbers, filename, shas, destpath, stoi("1"), TRACKER_UPDATE_DOWNLOAD_PORT, groupid);
				debug("MAINTHREAD");
				thr.detach();
				// connections[i].swap(thr);
			}

		}


		else if(s == "logout"){
			string buffer = s+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else{
				cout << "LOGGED OUT SUCCESSFULLY" << endl;
				break;
			}
		}

		else if(s == "show_downloads"){
			string buffer = s+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else{
				send(sock, "1", 1024, 0);
				char buffer[1024] = {0};
				read(sock, buffer, 1024);
				send(sock, "1", 1024, 0);
				int download_complete_size = stoi(buffer);
				for(int i=0; i<download_complete_size; i++){
					char buffer[1024] = {0};
					read(sock, buffer, 1024);
					send(sock, "1", 1024, 0);
					vector<string> v = split(buffer, ';');
					cout << "[C] [" << v[0] << "] " << v[1] << "\n";
				}

				char buffer2[1024] = {0};
				read(sock, buffer2, 1024);
				send(sock, "1", 1024, 0);
				int downloading_size = stoi(buffer2);
				for(int i=0; i<downloading_size; i++){
					char buffer[1024] = {0};
					read(sock, buffer, 1024);
					send(sock, "1", 1024, 0);
					vector<string> v = split(buffer, ';');
					cout << "[D] [" << v[0] << "] " << v[1] << "\n";
				}

			}
		}

		else if(s == "stop_share"){
			string groupid, file_name;
			cin >> groupid >> file_name;	
			string buffer = s+delimiter+groupid+delimiter+file_name+delimiter;
			send(sock, buffer.c_str(), buffer.size(), 0);

			char successbuffer[1024];
			int success = read(sock, successbuffer, 1024);
			// debug(successbuffer[0]);

			if(successbuffer[0] == '0'){
				cout << "YOU MUST BE LOGGED IN\n";
			}
			else if(successbuffer[0] == '1'){
				cout << "GROUP DOES NOT EXIST\n";
			}
			else if(successbuffer[0] == '2'){
				cout << "YOU ARE NOT A MEMBER OF THAT GROUP\n";
			}
			else if(successbuffer[0] == '3'){
				cout << "FILE DOES NOT EXIST IN THAT GROUP\n";
			}
			else if(successbuffer[0] == '4'){
				cout << "YOU ARE NOT SHARING THAT FILE ANYWAY\n";
			}
			else{
				cout << "STOPPED SHARING FILE\n";
			}
		}

		else{
			cout << "INVALID COMMAND\n";
		}
	}

	return 0;
}

//if there is a ; or space in uname or password or gid
//file names or paths must not contain spaces;
