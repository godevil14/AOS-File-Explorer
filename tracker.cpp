#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <bits/stdc++.h>
#include <sys/stat.h>

#define int long long

using namespace std;
int PORT, PORT2;

#include "debug.cpp"

string delimiter=";";


class File{
public:
	string name;
	int filesize;
	vector<string> sha;

	File(string file_name){
		name = file_name;
	}

	bool operator < (const File &f) const{
		return name < f.name;
	}
};


class Member{
public:
	string uname, ip;
	string port;
	set<File> uploads;

	Member(){

	}

	Member(string uname){
		this->uname = uname;
	}

	bool operator <(const Member &m) const{
		return uname < m.uname;
	}
	bool operator ==(Member &m){
		return uname == m.uname;
	}
	bool operator !=(Member &m){
		return uname != m.uname;
	}
};

class Group{
public:
	string id;
	Member owner;
	set<Member> participants;
	// map<File, vector<Member>> seeders;
	// map<Chunk, vector<Member>> file_accesses;
	set<Member> pending_requests;

	//bitmap[s][m][i] if chunk i of file with name s is with m

	map<string, map<string, vector<int>>> bitmap;
	set<File> available_files;

	Group(){

	}

	Group(string gid, Member &m){
		id = gid;
		owner = m;
		participants.insert(m);
	}
};


map<string, Group> groups;
set<string> user_logged;
map<string, set<pair<string, string>>> download_complete; //first groupid, second filename
map<string, set<pair<string, string>>> downloading;

mutex modify_credentials, modify_groups, modify_user_logged, modify_downloads;


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



void track_download(int new_socket){
	int lol;
	char buffer[1024];
	lol = read(new_socket, buffer, 1024);
	if(lol <= 0) return;
	vector<string> v = split(buffer, ';');
	string file_name = v[0];
	string groupid = v[1];
	string membername = v[2];
	int number_of_chunks = stoi(v[3]);
	int received = 0;
	send(new_socket, "1", 1024, 0);
	while(received != number_of_chunks){
		char buffer[1024];
		lol = read(new_socket, buffer, 1024);
		if(lol <= 0) break;
		received++;

		modify_groups.lock();
		groups[groupid].bitmap[file_name][membername].push_back(stoi(buffer)); // push the chunk number which is completed

		// debug(membername);
		// print(groups[groupid].bitmap[file_name][membername]);
		int one = groups[groupid].bitmap[file_name][membername].size();
		int two = groups[groupid].available_files.find(file_name)->sha.size();
		modify_groups.unlock();
		// debug(one, two);
		if(one == two){
			modify_downloads.lock();
			downloading[membername].erase({groupid, file_name});
			download_complete[membername].insert({groupid, file_name});
			modify_downloads.unlock();
			send(new_socket, "1", 1024, 0);
		}
		else{
			send(new_socket, "0", 1024, 0);
		}
		// debug(received);
	}
}

void listen_for_completed_chunks(string tracker_ip){
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(tracker_ip.c_str());
	address.sin_port = htons(PORT2);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	vector<thread> clients;
	while(true){
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		clients.push_back(thread(track_download, new_socket));

	}
}

int check_if_existing_user(ifstream &cred, string &uname, string &pass){
	//return 0 if doesnt exist
	//return 1 if only uname matches
	//return 2 if both match
	string s;
	string tempuname, temppass;
	int exists = 0;
	while(getline(cred, s)){
		vector<string> spl = split(s, ';');
		tempuname = spl[0]; temppass = spl[1];
		if(tempuname == uname && temppass == pass){
			exists = 2;
			break;
		}
		else if(tempuname == uname){
			exists = 1;
		}
	}
	return exists;
}

void client_thread(int new_socket){
	int loggedin = 0;
	Member member;
	while(true){
		char buffer[1024] = { 0 };
		int valread = read(new_socket, buffer, 1024);
		if(!valread)
			break;
		vector<string> commands = split(buffer, ';');
		print(commands);
		if(commands[0] == "create_user"){
			debug("Creating User");
			string uname = commands[1], pass = commands[2];
			// debug(uname, pass);
			modify_credentials.lock();
			ifstream cred("credentials.txt");
			int exists = check_if_existing_user(cred, uname, pass);
			modify_credentials.unlock();

			int success_code;
			//0 for user already exists
			//1 for successful creation

			if(exists){
				cout << "USER ALREADY EXISTS\n";
				success_code = 0;
			}
			else{
				// debug(uname, pass);
				modify_credentials.lock();
				ofstream cred2("credentials.txt", ios::app);
				cred2 << uname << ';' << pass << "\n";
				modify_credentials.unlock();
				cout << "USER CREATED\n";
				success_code = 1;
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

		else if(commands[0] == "login"){
			debug("Doing login");
			string uname = commands[1], pass = commands[2];

			modify_user_logged.lock();
			int flag = 0;
			if(user_logged.find(uname) != user_logged.end()){
				cout << "ALREADY LOGGED IN\n";
				send(new_socket, to_string(2).c_str(), 1024, 0);
				flag = 1;
			}
			modify_user_logged.unlock();
			if(flag == 1)
				continue;

			modify_credentials.lock();
			ifstream cred("credentials.txt");
			int exists = check_if_existing_user(cred, uname, pass);
			modify_credentials.unlock();

			int success_code;
			//0 for invalid creds
			//1 for successful login

			if(exists == 2){
				cout << "LOGIN SUCCESS\n";
				success_code = 1;
				loggedin = 1;
				Member m(uname);
				member = m;
				member.ip = commands[3];
				member.port = commands[4];
				modify_user_logged.lock();
				user_logged.insert(uname);
				modify_user_logged.unlock();
				debug(member.ip, member.port, member.uname);

				ifstream ifile("tracker1/" + uname);
				string grp_name;
				while(ifile >> grp_name){
					string file_name;
					ifile >> file_name;
					int no_of_chunks;
					ifile >> no_of_chunks;
					vector<int> chunks_numbers(no_of_chunks);
					for(int i=0; i<no_of_chunks; i++)
						ifile >> chunks_numbers[i];
					groups[grp_name].bitmap[file_name][uname] = chunks_numbers;
				}
			}
			else{
				cout << "INVALID CREDENTIALS\n";
				success_code = 0;
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

		else if(commands[0] == "create_group"){
			debug("Creating Group");
			string groupid = commands[1];

			int success_code;
			//0 if not logged in
			//1 for group already exists
			//2 for successful creation

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) != groups.end()){
				cout << "GROUP ALREADY EXISTS\n";
				success_code = 1;
			}
			else{
				Group g(groupid, member);
				modify_groups.lock();
				groups[groupid] = g;
				modify_groups.unlock();
				cout << "GROUP CREATED SUCCESSFULLY\n";
				success_code = 2;
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

		else if(commands[0] == "join_group"){
			string groupid = commands[1];

			int success_code;
			//0 if not logged in
			//1 for group doesnt exist
			//2 already have a pending request
			//3 successful request sent
			//4 you already belong to that group

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member) != groups[groupid].participants.end()){
				cout << "YOU ALREADY BELONG TO THE GROUP\n";
				success_code = 4;
			}
			else if(groups[groupid].pending_requests.find(member) != groups[groupid].pending_requests.end()){
				cout << "YOU ALREADY HAVE A PENDING REQUEST\n";
				success_code = 2;
			}
			else{
				// debug(member.uname);
				modify_groups.lock();
				groups[groupid].pending_requests.insert(member);
				modify_groups.unlock();
				cout << "REQUEST SENT SUCCESSFULLY\n";
				success_code = 3;
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
			debug(member.uname);
		}

		else if(commands[0] == "leave_group"){
			string groupid = commands[1];

			int success_code;
			//0 if not logged in
			//1 for group doesnt exist
			//2 do not belong to the group
			//3 successfully left

			debug(groupid);
			for(auto x: groups[groupid].participants)
				debug(x.uname);

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member.uname) == groups[groupid].participants.end()){
				cout << "YOU DO NOT BELONG TO THE GROUP\n";
				success_code = 2;
			}
			else{
				cout << "REMOVING MEMBER FROM GROUP\n";
				success_code = 3;
				vector<string> removed_files;
				for(auto x: groups[groupid].bitmap){
					if(x.second.find(member.uname) != x.second.end()){
						x.second.erase(member.uname);
					}
					if(x.second.size() == 0){
						removed_files.push_back(x.first);
					}
				}
				for(auto x: removed_files){
					groups[groupid].bitmap.erase(x);
					groups[groupid].available_files.erase(x);
				}
				groups[groupid].participants.erase(member.uname);
				if(groups[groupid].participants.size() == 0){ //everyone in group left
					groups.erase(groupid);
				}
				else if(groups[groupid].owner.uname == member.uname){ //change owner
					groups[groupid].owner = *groups[groupid].participants.begin();
				}
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

		else if(commands[0] == "list_requests"){
			string groupid = commands[1];

			int success_code;
			// 0 if not logged in
			// 1 for group doesnt exist
			// 2 invalid perms
			// 3 show list

			// debug(member.uname);

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].owner != member){
				cout << "YOU ARE NOT THE OWNER OF THIS GROUP\n";
				success_code = 2;
			}
			else{
				cout << "SENDING PENDING LIST\n";
				success_code = 3;
				// debug(groups[groupid].owner.uname);
			}

			char buffer2[1024];
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
			int ack = read(new_socket, buffer2, 1024);

			if(buffer2[0] == '1'){
				string list;
				for(auto x: groups[groupid].pending_requests){
					list += x.uname;
					list += ';';
				}
				// debug(list);
				send(new_socket, list.c_str(), 1024, 0);
			}
		}

		else if(commands[0] == "accept_request"){
			string groupid = commands[1];
			string userid = commands[2];

			Member searchm(userid);

			int success_code;
			// 0 if not logged in
			// 1 for group doesnt exist
			// 2 invalid perms
			// 3 not in pending list
			// 4 accept request

			// debug(member.uname);

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].owner != member){
				cout << "YOU ARE NOT THE OWNER OF THIS GROUP\n";
				success_code = 2;
			}
			else if(groups[groupid].pending_requests.find(searchm) == groups[groupid].pending_requests.end()){
				cout << "THERE IS NO REQUEST FROM THAT USER\n";
				success_code = 3;
			}
			else{
				cout << "USER SUCCESSFULLY ADDED TO GROUP\n";
				modify_groups.lock();
				auto ptr = groups[groupid].pending_requests.find(searchm);
				groups[groupid].participants.insert(*ptr);
				groups[groupid].pending_requests.erase(searchm);
				// for(auto &x: groups[groupid].available_files){
				// 	groups[groupid].bitmap[x.name][userid].resize(x.sha.size());
				// }
				modify_groups.unlock();
				success_code = 4;
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

		else if(commands[0] == "list_groups"){
			int success_code;
			// 0 if not logged in
			// 1 show groups

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else{
				cout << "SENDING GROUP LIST\n";
				success_code = 1;	
			}
			char buffer2[1024];
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
			int ack = read(new_socket, buffer2, 1024);

			for(auto x: groups){
				debug(x.first);
			}

			if(buffer2[0] == '1'){
				string list;
				for(auto x: groups){
					list += x.first;
					list += ';';
				}
				// debug(list);
				send(new_socket, list.c_str(), 1024, 0);
			}
		}

		else if(commands[0] == "list_files"){
			string groupid = commands[1];

			int success_code;
			// 0 if not logged in 
			// 1 if group does not exist
			// 2 if you dont belong to group
			// 3 sending available files


			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member) == groups[groupid].participants.end()){
				cout << "YOU DO NOT BELONG TO THE GROUP\n";
				success_code = 2;
			}
			else{
				cout << "SENDING AVAILABLE FILES\n";
				success_code = 3;
			}

			char buffer2[1024];
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
			int ack = read(new_socket, buffer2, 1024);

			if(buffer2[0] == '1'){
				string list;
				for(auto x: groups[groupid].available_files){
					list += x.name;
					list += ';';
				}
				// debug(list);
				send(new_socket, list.c_str(), 1024, 0);
			}
		}

		else if(commands[0] == "upload_file"){
			string file_name = commands[1];
			string groupid = commands[2];
			int success_code;
			// 0 if not logged in
			// 1 group does not exist
			// 2 not a group member
			// 3 file already exists in group
			// 4 uploading file

			debug(file_name, groupid);

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member) == groups[groupid].participants.end()){
				cout << "YOU DO NOT BELONG TO THE GROUP\n";
				success_code = 2;
			}
			else if(groups[groupid].bitmap.find(file_name) != groups[groupid].bitmap.end()){
				cout << "THE FILE ALREADY EXISTS IN THAT GROUP\n";
				success_code = 3;
			}
			else{
				cout << "UPLOADING FILE\n";
				success_code = 4;
			}
			// debug(success_code);
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
			if(success_code == 4){

				for(auto x: groups){
					debug(x.first);
				}

				File f(file_name);
				char chunkscstr[1024];
				read(new_socket, chunkscstr, 1024);
				send(new_socket, "1", 1024, 0);
				string chunksstr = chunkscstr;
				int chunks = stoi(chunksstr);
				for(int i=0; i<chunks; i++){
					char shaarr[1024];
					read(new_socket, shaarr, 1024);
					send(new_socket, "1", 1024, 0);
					// debug(shaarr);
					f.sha.push_back(shaarr);
				}
				char file_size[1024];
				read(new_socket, file_size, 1024);
				send(new_socket, "1", 1024, 0);

				f.filesize = stoll(file_size);

				for(auto x: groups){
					debug(x.first);
				}

				
				

				// char fullhashchunkscstr[1024];
				// read(new_socket, fullhashchunkscstr, 1024);
				// send(new_socket, "1", 1024, 0);
				// string fullhashchunksstr = fullhashchunkscstr;
				// int fullhashchunks = stoi(fullhashchunksstr);

				// string fullhash;
				// debug(fullhashchunks);
				// for(int i=0; i<fullhashchunks; i++){
				// 	char shapart[1024];
				// 	read(new_socket, shapart, 1024);
				// 	send(new_socket, "1", 1024, 0);
				// 	fullhash += shapart;
				// 	debug(shapart);
				// }
				// debug(fullhash);

				modify_groups.lock();
				// groups[groupid].bitmap[file_name][member.uname].resize(f.sha.size());
				// for(auto &x: groups[groupid].bitmap[file_name][member.uname]){
				// 	x = 1;
				// }

				//THIS IS REALLY UNSAFE FOR MULTITHREAD
				for(int i=0; i<f.sha.size(); i++){
					groups[groupid].bitmap[file_name][member.uname].push_back(i);
				}
				groups[groupid].available_files.insert(f);
				modify_groups.unlock();

				// for(auto x: groups){
				// 	debug(x.first);
				// }

				for(auto x: groups){
					debug(x.first);
				}

			}
		}

		else if(commands[0] == "download_file"){
			string groupid = commands[1];
			string file_name = commands[2];
			int success_code;
			// 0 if not logged in
			// 1 group does not exist
			// 2 not a group member
			// 3 file does not exist in group
			// 4 sending peer info


			debug(file_name, groupid);
			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member) == groups[groupid].participants.end()){
				cout << "YOU DO NOT BELONG TO THE GROUP\n";
				success_code = 2;
			}
			else if(groups[groupid].bitmap.find(file_name) == groups[groupid].bitmap.end()){
				for(auto x: groups[groupid].bitmap){
					cout << x.first << "\n";
				}
				cout << "THE FILE DOES NOT EXIST IN THAT GROUP\n";
				success_code = 3;
			}
			else{
				cout << "SENDING PEER INFO\n";
				success_code = 4;
			}

			char buffer[1024];
			send(new_socket, to_string(success_code).c_str(), 1024, 0);

			if(success_code != 4){
				continue;
			}
			read(new_socket, buffer, 1024);
			// debug("GGGGGGGGGGGGGG");

			// debug(success_code);

			// send(new_socket, to_string(groups[groupid].available_files.find(file_name) -> filesize).c_str(), 1024, 0);
			// read(new_socket, buffer, 1024);

			// for(auto x: groups[groupid].participants){
			// 	debug(x.ip, x.port, x.uname);
			// }

			// for(auto x: groups[groupid].bitmap[file_name]){
			// 	debug(x.first);
			// 	print(x.second);
			// }

			set<int> done;
			map<pair<string, string>, vector<string>> ipportchunk;
			for(auto &x: groups[groupid].bitmap[file_name]){
				if(x.first == member.uname)
					continue; //dont take chunks from yourself
				Member temp(x.first);
				auto mem = groups[groupid].participants.find(temp);
				debug(x.first, mem->ip, mem->port);
				debug(x.second.size());
				for(auto &z: x.second){
					if(done.find(z) == done.end() && user_logged.find(mem->uname) != user_logged.end()){
						done.insert(z);
						ipportchunk[{mem->ip, mem->port}].push_back(to_string(z));
					}
				}
			}

			modify_downloads.lock();
			downloading[member.uname].insert({groupid, file_name});
			modify_downloads.unlock();

			// for(auto x: ipportchunk){
			// 	print(x.first);
			// 	debug(x.second.size());
			// }
			// debug("VV");
			// print(chunkipport);
			char buffer2[1024];
			// debug(to_string(ipportchunk.size()).c_str());
			send(new_socket, to_string(ipportchunk.size()).c_str(), 1024, 0);
			read(new_socket, buffer2, 1024);
			for(auto x: ipportchunk){
				char buffer[1024];
				// debug(x.first.first, x.first.second, x.second.size());
				send(new_socket, (x.first.first+delimiter+x.first.second+delimiter+to_string(x.second.size())+delimiter).c_str(), 1024, 0);
				read(new_socket, buffer, 1024);
				for(auto y: x.second){
					char buffer[1024];
					send(new_socket, y.c_str(), 1024, 0);
					read(new_socket, buffer, 1024);
				}
				// File f = *groups[groupid].available_files.find(file_name);
				// for(auto y: f.sha){
				// 	char buffer[1024];
				// 	send(new_socket, y.c_str(), 1024, 0);
				// 	read(new_socket, buffer, 1024);
				// }
			}
		}

		else if(commands[0] == "logout"){
			int success_code;
			// 0 if not logged in
			// 1 logging out

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
				send(new_socket, to_string(0).c_str(), 1024, 0);
			}
			else{
				success_code = 1;
				modify_user_logged.lock();
				user_logged.erase(member.uname);
				modify_user_logged.unlock();
				modify_groups.lock();
				ofstream memberdata("tracker1/"+member.uname);


				// for(auto x: groups["abc"].bitmap["Algebra_Artin.pdf"]){
				// 	debug(x.first);
				// 	print(x.second);
				// }
				for(auto &x: groups){
					for(auto &y: x.second.bitmap){
						memberdata << x.first << "\n"; //group name
						memberdata << y.first << "\n"; //file name
						debug(x.first, y.first, member.uname);
						memberdata << y.second[member.uname].size() << "\n"; //number of chunks
						for(auto &z: y.second[member.uname]){
							memberdata << z << " "; //chunk numbers
						}
						memberdata << "\n";
						y.second[member.uname].clear();
					}
				}
				// debug("KK");
				// for(auto x: groups["abc"].bitmap["Algebra_Artin.pdf"]){
				// 	debug(x.first);
				// 	print(x.second);
				// }
				modify_groups.unlock();
				loggedin = 0;
				send(new_socket, to_string(1).c_str(), 1024, 0);	
				break;
			}
		}

		else if(commands[0] == "show_downloads"){
			int success_code;
			// 0 if not logged in
			// 1 show downloads

			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else{
				cout << "SHOWING DOWNLOADS\n";
				success_code = 1;
			}
			char buffer[1024];
			send(new_socket, to_string(success_code).c_str(), 1024, 0);

			if(success_code == 1){
				char ack[1024] = {0};
				read(new_socket, ack, 1024);

				modify_downloads.lock();
				int complete_downloads_size = download_complete[member.uname].size();
				send(new_socket, to_string(complete_downloads_size).c_str(), 1024, 0);
				read(new_socket, ack, 1024);
				for(auto x: download_complete[member.uname]){
					char buffer[1024] = {0};
					send(new_socket, (x.first+";"+x.second).c_str(), 1024, 0);
					read(new_socket, buffer, 1024);
				}

				int downloading_size = downloading[member.uname].size();
				send(new_socket, to_string(downloading_size).c_str(), 1024, 0);
				read(new_socket, buffer, 1024);
				for(auto x: downloading[member.uname]){
					char buffer[1024] = {0};
					send(new_socket, (x.first+";"+x.second).c_str(), 1024, 0);
					read(new_socket, buffer, 1024);
				}
				modify_downloads.unlock();
			}
			
		}

		else if(commands[0] == "stop_share"){
			string groupid = commands[1];
			string file_name = commands[2];
			int success_code;
			// 0 if not logged in
			// 1 group does not exist
			// 2 not a group member
			// 3 file does not exist in group
			// 4 not sharing anyway
			// 5 sending peer info

			debug(file_name, groupid);
			if(loggedin == 0){
				cout << "YOU MUST BE LOGGED IN\n";
				success_code = 0;
			}
			else if(groups.find(groupid) == groups.end()){
				cout << "GROUP DOES NOT EXISTS\n";
				success_code = 1;
			}
			else if(groups[groupid].participants.find(member) == groups[groupid].participants.end()){
				cout << "YOU DO NOT BELONG TO THE GROUP\n";
				success_code = 2;
			}
			else if(groups[groupid].bitmap.find(file_name) == groups[groupid].bitmap.end()){
				cout << "THE FILE DOES NOT EXIST IN THAT GROUP\n";
				success_code = 3;
			}
			else if(groups[groupid].bitmap[file_name].find(member.uname) == groups[groupid].bitmap[file_name].end()){
				cout << "NOT SHARING THE FILE ANYWAY\n";
				success_code = 4;
			}
			else{
				cout << "STOPPED SHARING THE FILE\n";
				success_code = 5;
				groups[groupid].bitmap[file_name].erase(member.uname);
				if(groups[groupid].bitmap[file_name].size() == 0){
					groups[groupid].bitmap.erase(file_name);
					groups[groupid].available_files.erase(file_name);
				}
			}
			send(new_socket, to_string(success_code).c_str(), 1024, 0);
		}

	}

}

void test(){
	int chunksize = 512*1024;
	Member m("tt"), m2("hh"), m3("aa");
	m.ip = "5.5.5.5";
	m.port = "7070";
	m2.ip = "5.5.5.5";
	m2.port = "9090";
	m3.ip = "5.5.5.5";
	m3.port = "6060";
	Group g("abc", m);
	g.participants.insert(m2);
	g.participants.insert(m3);
	// user_logged.insert("tt");
	// user_logged.insert("uu");
	int f1size = 393856200, f2size = 9034132, f3size = 3826831360;
	File f1("Mongo_Tutorial.mov"), f2("Algebra_Artin.pdf"), f3("ubuntu.iso");
	for(int i=0; i < f1size/chunksize + (f1size%chunksize != 0); i++){
		g.bitmap[f1.name][m.uname].push_back(i);
		f1.sha.push_back("FF");
	}
	for(int i=0; i < f2size/chunksize + (f2size%chunksize != 0); i++){
		g.bitmap[f2.name][m.uname].push_back(i);
		f2.sha.push_back("GG");
	}
	for(int i=0; i < f3size/chunksize + (f3size%chunksize != 0); i++){
		g.bitmap[f3.name][m.uname].push_back(i);
		f3.sha.push_back("GG");
	}

	// for(int i=0; i<300; i++){
	// 	g.bitmap[f1.name][m.uname].push_back(i);
	// }
	// for(int i=300; i<752; i++){
	// 	g.bitmap[f1.name][m2.uname].push_back(i);
	// }


	g.available_files.insert(f1);
	g.available_files.insert(f2);
	g.available_files.insert(f3);
	// print(g.bitmap[f1.name][m.uname]);
	// print(g.bitmap[f2.name][m.uname]);
	groups["abc"] = g;
}

void listen_for_quit(){
	string s;
	cin >> s;
	if(s == "quit")
		exit(0);
}

int32_t main(int32_t argc, char const* argv[])
{
	if(argc != 3){
		cout << "INVALID NUMBER OF ARGUMENTS\n";
		return 0;
	}

	// test();

	int tracker_number = stoi(argv[2]);

	ifstream tracker_info(argv[1]);
	string tracker_ip;
	tracker_info >> tracker_ip >> PORT >> PORT2;
	if(tracker_number == 2)
		tracker_info >> tracker_ip >> PORT >> PORT2;

	debug(tracker_ip, PORT, PORT2);

	thread quit(listen_for_quit);
	thread lfrc(listen_for_completed_chunks, tracker_ip);

	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(tracker_ip.c_str());
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	struct stat info;
	if(stat("tracker1", &info )!= 0){
		mkdir("tracker1", 0777);
	}


	vector<thread> clients;
	while(true){
		if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		// debug(new_socket);
		clients.push_back(thread(client_thread, new_socket));

	}
	
	
	
	// for(auto &x: clients){
	// 	x.join();
	// }

	// closing the connected socket
	// close(new_socket);
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
