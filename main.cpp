#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <map>
#include <fstream>
#include <random>
#include <chrono>  
#include <gperftools/profiler.h> 
#include "heavykeeper.h"
using namespace std;

#define HK_d 2 // maximum memory size
map <string ,int> B,C;
struct node {string x;int y;} p[10000005];
ifstream fin("u1",ios::in|ios::binary);
char a[105];
string Read()
{
    fin.read(a,13);
    a[13]='\0';
    string tmp=a;
    return tmp;
}
int cmp(node i,node j) {return i.y>j.y;}


std::random_device rd;
std::mt19937 gen(rd());
 
int random(int low, int high)
{
    std::uniform_int_distribution<> dist(low, high);
    return dist(gen);
}


int main()
{
    int K = 100;
    int m = 10000;  // the number of flows
    int hot_len = 30;
    int MEM = 100;

    int hk_M;
    // https://planetmath.org/goodhashtableprimes 选择6151作为大小哈希表大小
    // 96*hk_M*HK_d是heavykeeper的内存；192*K是LRU的内存；没有计算LRU中哈希表的内存
    // 
    for (hk_M=1; 96*hk_M*HK_d+192*K<=MEM*1024*8; hk_M++); 
    if (hk_M%2==0) hk_M--;

    // 哈希表长度     内存使用量（不加哈希表）
    // 6151         146KB
    // 3079         74KB
    std::cout << hk_M << " ======================== " <<(3079*96*2+100*192)/8/1024 << std::endl;;
    

    HeavyKeeper *hk; 
    hk = new HeavyKeeper(K);
    hk->Clear();
    
    auto changeValue  = [](size_t para) -> string {
        return to_string(para) + "a";
    };
    int sum = 0;
    // Inserting随机数
    
    std::vector<std::string> string_array;
    for (size_t i=0; i<=m; i++) {
        string_array.push_back(changeValue(i));
        B[string_array[i]]++;
        sum++;
    }

    for (size_t i=0; i<=hot_len; i++) {
        size_t s = random(1, m);
        //std::cout << "lizhaolong " << changeValue(s) << std::endl;
        for (size_t j=0; j<=10000 + s; j++) {
            string_array.push_back(changeValue(s));
            B[changeValue(s)]++;
            sum++;
        }
    }


    auto start = std::chrono::steady_clock::now();
    ProfilerStart("test.prof");
    // for (size_t i=0; i<=m; i++) {
    //     // B中存的是实际的请求和请求数
	// 	hk->Insert(string_array[i]);
    //     sum++;
	// }

    // for (size_t i=0; i<=hot_len; i++) {
    //     size_t s = random(1, m);
    //     //std::cout << "lizhaolong " << changeValue(s) << std::endl;
    //     for (size_t j=0; j<=10000 + s; j++) {
    //         B[changeValue(s)]++;
    //         hk->Insert(changeValue(s));
    //         sum++;
    //     }
    // }
    for (const auto& item : string_array) {
        hk->Insert(item);
    }

    ProfilerStop();
    auto end = std::chrono::steady_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << duration << "毫秒; " << sum << "个请求" << std::endl;
	hk->Work();


    cout<<"preparing true flow"<<endl;
	// preparing true flow
	int cnt=0;
    for (map <string,int>::iterator sit=B.begin(); sit!=B.end(); sit++) {   
        // p存的实际是B的所有值，有什么意义。
        p[++cnt].x=sit->first;
        p[cnt].y=sit->second;
    }
    // p中存着热点key的最大到最小
    sort(p+1,p+cnt+1,cmp);

    for (int i=1; i<=K; i++) {
        std::cout << "p[i].x [" << p[i].x << "] p[i].y [ " << p[i].y << "]\n";
        C[p[i].x]=p[i].y;
    }

    // Calculating PRE, ARE, AAE
    cout<<"Calculating"<<endl;
    int hk_sum=0,hk_AAE=0;
    double hk_ARE=0;
    string hk_string; int hk_num;

    for (int i=0; i<K; i++) {
        hk_string=(hk->Query(i)).first; 
        hk_num=(hk->Query(i)).second;
        std::cout << hk_string << " hk_num[" << hk_num << "];  B[hk_string] [" << B[hk_string] << "]" << std::endl;
        // 所有key统计的sum和实际的sum差了多少
        hk_AAE+=abs(B[hk_string]-hk_num); 
        // 每个key误判的个数和总数的比例
        hk_ARE+=abs(B[hk_string]-hk_num)/(B[hk_string]+0.0);
        // 实际识别到的热点数
        if (C[hk_string]) hk_sum++;
    }

    // for (int i = 0; i < 1000; i+=10) {
    //     std::cout << i << " " << double(std::pow(1.08, -i)) << " " << int(pow(1.08, i)) << std::endl;
    // }
    printf("heavkeeper:\nAccepted: %d/%d  %.10f\nARE: %.10f\nAAE: %.10f\n\n",hk_sum,K, (hk_sum/(K+0.0)), hk_ARE/K, hk_AAE/(K+0.0));
    return 0;
}

// g++ -g -std=c++17  main.cpp -lprofiler
// pprof ./a.out test.prof --pdf > test.pdf