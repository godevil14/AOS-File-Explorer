// #include <bits/stdc++.h>
// using namespace std;

//https://codeforces.com/blog/entry/76087
vector<string> vec_splitter(string s) {
    s += ',';
    vector<string> res;
    while(!s.empty()) {
        res.push_back(s.substr(0, s.find(',')));
        s = s.substr(s.find(',') + 1);
    }
    return res;
}
void debug_out(
vector<string> __attribute__ ((unused)) args,
__attribute__ ((unused)) int idx, 
__attribute__ ((unused)) int LINE_NUM) { cerr << endl; } 
template <typename Head, typename... Tail>
void debug_out(vector<string> args, int idx, int LINE_NUM, Head H, Tail... T) {
    if(idx > 0) cerr << ", "; else cerr << "Line(" << LINE_NUM << ") ";
    stringstream ss; ss << H;
    cerr << args[idx] << " = " << ss.str();
    debug_out(args, idx + 1, LINE_NUM, T...);
}


#ifndef ONLINE_JUDGE
#define debug(...) debug_out(vec_splitter(#__VA_ARGS__), 0, __LINE__, __VA_ARGS__)
#else
#define debug(...) 42
#endif


template <typename T>
void print(T arr[], int n){
	cout << "[";
	for(int i=0; i<n; i++)
		cout << arr[i] << ", ";
	cout << "]" << endl;
}

//vector<int, char, string...>
template <typename T>
void print(vector<T> v){
	cout << "[";
	for(auto x: v)
		cout << x << ", ";
	cout << "]" << endl;
}

template <typename T>
void print(vector<vector<T>> v){
	cout << "[";
	for(auto x: v){
		cout << "[";
		for(auto y: x)
			cout << y << ", ";
		cout << "], ";
	}
	cout << "]" << endl;
}

template <typename T1, typename T2>
void print(pair<T1, T2> p){
	cout << "[";
	cout << p.first << ", " << p.second << "]" << endl;
}

template <typename T1, typename T2>
void print(vector<pair<T1, T2>> v){
	cout << "[";
	for(auto x: v){
		cout << "[";
		cout << x.first << ", " << x.second << "], ";
	}
	cout << "]" << endl;
}

template <typename T>
void print(set<T> v){
	cout << "{";
	for(auto x: v)
		cout << x << ", ";
	cout << "}" << endl;
}

template <typename T1, typename T2>
void print(set<pair<T1, T2>> sp){
	cout << "{ ";
	for(auto x: sp)
		cout << "[" << x.first << ", " << x.second << "]" << ", ";
	cout << "}" << endl;
}

template <typename T>
void print(multiset<T> v){
	cout << "{";
	for(auto x: v)
		cout << x << ", ";
	cout << "}" << endl;
}

template <typename T1, typename T2>
void print(map<T1, T2> mp){
	cout << "{";
	for(auto x: mp){
		cout << x.first << ":" << x.second << ", ";
	}
	cout << "}" << endl;
}


// int main(){
// 	multiset<int> arr = {1, 1, 2, 4};
// 	print(arr);
// }