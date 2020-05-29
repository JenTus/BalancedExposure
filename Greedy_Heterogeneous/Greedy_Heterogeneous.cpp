/*---------------------------------------------------------------
 | Computes 2-edge connected conponents of a graph
 *---------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
#include "rfw_timer.h"

using namespace std;

#define MAXLINE       500   /* max length of input line */
#define K             1000
//#define NUM_TO_SELECT 5
char line[MAXLINE]; /* stores input line */

long int rand_seeds[K];


map<string, int> user2ID;
vector<int> InputEdges;
vector<double> EdgeProb_a;
vector<double> EdgeProb_b;
vector<int> a_seeds;
vector<int> b_seeds;

int *queue;
int *temp_stack;
int *temp_stack_2;

int n, m; /* number of nodes, arcs */
int *Gout_full, *GfirstOut_full, *GlastOut_full; // adjacency list
int *Gin_full, *GfirstIn_full, *GlastIn_full; // adjacency list
int *Gout_a, *GfirstOut_a, *GlastOut_a; // adjacency list
int *Gout_b, *GfirstOut_b, *GlastOut_b; // adjacency list

inline string intToStr(int i) {
    stringstream sx;
    sx << i;
    return sx.str();
}

inline unsigned int strToInt(string s) {
    unsigned int i;
    istringstream myStream(s);

    if(myStream >> i)
        return i;

    else {
        std::cout << "String " << s << " is not a number." << endl;
        return atoi(s.c_str());
    }

    return i;
}

float getCurrentMemoryUsage() { //megabyte verii
    string pid = intToStr(unsigned(getpid()));
    string outfile = "temp/tmp_" + pid + ".txt";
    string command = "pmap " + pid + " | grep -i Total | awk '{print $2}' > " + outfile;
    system(command.c_str());

    string mem_str;
    ifstream ifs(outfile.c_str());
    std::getline(ifs, mem_str);
    ifs.close();

    mem_str = mem_str.substr(0, mem_str.size()-1);
    float mem = (float)strToInt(mem_str);

    return mem/1024; // in MB
}


int reachable_from_initial_seeds(int *GfirstOut, int* Gout, bool* reachable_cascade, bool* reachable, vector<int> &seeds){
    
    int queueHead, queueTail, selected, temp;
    int num_of_reachable_vertices = 0;
  
    for(int i=0; i<= n; i++) reachable[i] = reachable_cascade[i] = false;
    
    //printf("seeds:%d\n",(int)seeds.size());
    for(unsigned i=0; i< seeds.size(); i++){
		int source = seeds.at(i);
		
		queueHead = queueTail = 0;
	
		if(!reachable_cascade[source]){
		    queue[queueTail++] = source;
		    reachable_cascade[source] = true;
		    //num_of_reachable_vertices++;
		}
	
		//printf("reachable num after  %d-th seed: %d\n",i+1,num_of_reachable_vertices);
		while(queueHead != queueTail){
		    selected = queue[queueHead++];
		    for(int i = GfirstOut[selected]; i < GfirstOut[selected+1]; i++){
				temp = Gout[i];
				if(!reachable_cascade[temp]){
				    queue[queueTail++] = temp;
				    reachable_cascade[temp] = true;
				    //num_of_reachable_vertices++;
				}
		    }
		    if(!reachable[selected]){
	    		reachable[selected] = true;
			    num_of_reachable_vertices++;
			}
			// scan all vertices that follows selected
			for(int j = GfirstIn_full[selected]; j < GfirstIn_full[selected+1]; j++){
		    	// mark any new exposed vertices
		    	if(!reachable[Gin_full[j]]){
		    		reachable[Gin_full[j]] = true;
				    num_of_reachable_vertices++;
				}
			}
		}
	}
    return num_of_reachable_vertices;
}

int reachable_from_vertex(int v, int *GfirstOut, int* Gout, bool* reachable_cascade_same, bool* reachable_same, bool* reachable_other){
    
    int queueHead, queueTail, selected, temp, temp_stack_top, temp_stack_2_top;
    int num_of_reachable_vertices = 0;
    
    //printf("seeds:%d\n",(int)seeds.size());

	queueHead = queueTail = temp_stack_top = temp_stack_2_top = 0;

	if(!reachable_cascade_same[v]){
	    queue[queueTail++] = v;
	    reachable_cascade_same[v] = true;
	    temp_stack_2[temp_stack_2_top++] = v;
	}

	//printf("reachable num after  %d-th seed: %d\n",i+1,num_of_reachable_vertices);
	while(queueHead != queueTail){
		selected = queue[queueHead++];
		for(int i = GfirstOut[selected]; i < GfirstOut[selected+1]; i++){
			temp = Gout[i];
			if(!reachable_cascade_same[temp]){
			    queue[queueTail++] = temp;
			    reachable_cascade_same[temp] = true;
			    temp_stack_2[temp_stack_2_top++] = temp;
			}
		}
		
		if(!reachable_same[selected]){
    		reachable_same[selected] = true;
	    	temp_stack[temp_stack_top++] = selected;
		    if(reachable_other[selected]) num_of_reachable_vertices++;
		}
		// scan all vertices that follows selected
		for(int j = GfirstIn_full[selected]; j < GfirstIn_full[selected+1]; j++){
	    	// mark any new exposed vertices
	    	if(!reachable_same[Gin_full[j]]){
	    		reachable_same[Gin_full[j]] = true;
			    if(reachable_other[Gin_full[j]]) num_of_reachable_vertices++;
		    	temp_stack[temp_stack_top++] = Gin_full[j];
			}
		}
	}
	
	for(int i = 0; i < temp_stack_top; i++){
		reachable_same[temp_stack[i]] = false;
	}
	for(int i = 0; i < temp_stack_2_top; i++){
		reachable_cascade_same[temp_stack_2[i]] = false;
	}
	
    return num_of_reachable_vertices;
}

void Greedy(int NUM_TO_SELECT){
	
	int x,y;
	
	
	//Gout_full = new int [m];
	//GfirstOut_full = new int [n+2];
	//GlastOut_full = new int [n+2];
	
	Gin_full = new int [m];
	GfirstIn_full = new int [n+2];
	GlastIn_full = new int [n+2];
	
	Gout_a = new int [m];
	GfirstOut_a = new int [n+2];
	GlastOut_a = new int [n+2];
	
	Gout_b = new int [m];
	GfirstOut_b = new int [n+2];
	GlastOut_b = new int [n+2];
	
	//Gin  = new int [m];
	//GfirstIn  = new int [n+2];
	//GlastIn = new int [n+2];
	
	srand(time(NULL));
    
	for (int i=0; i<K; i++){
		rand_seeds[i] = rand();
		//cout << i <<"-th random number:" << rand_seeds[i]<< endl;
	}
    
	vector<int> edges_sampled;
	bool *reachable_cascade_a = new bool [n+1];
	bool *reachable_a = new bool [n+1];
	bool *reachable_cascade_b = new bool [n+1];
	bool *reachable_b = new bool [n+1];
	queue = new int [n+1];
	temp_stack = new int [n+1];
	temp_stack_2 = new int [n+1];
	int *new_reachable_created_a = new int [n+1];
	int *new_reachable_created_b = new int [n+1];
	
	
	/*GlastOut_full[0] = */GlastIn_full[0] = 0;
	
	for (int j = 0; j <= n+1; j++){
		/*GfirstOut_full[j] = */GfirstIn_full[j] = 0;
	}
	
	for (unsigned j=0; j<InputEdges.size(); j+=2){
		x = InputEdges.at(j);
		y = InputEdges.at(j+1);
		//GfirstOut_full[x+1]++;
		GfirstIn_full[y+1]++;
		//printf("(%d,%d) exists\n",x,y);
	}
	
	for (int k = 1; k <= n+1; k++) {
		//GfirstOut_full[k] += GfirstOut_full[k-1]; // the +1 is to be able to add edges in the recidual
		//GlastOut_full[k] = GfirstOut_full[k];
		GfirstIn_full[k] += GfirstIn_full[k-1];
		GlastIn_full[k] = GfirstIn_full[k];
	}
	
	for (unsigned j=0; j<InputEdges.size(); j+=2){
		x = InputEdges.at(j);
		y = InputEdges.at(j+1);
		//Gout_full[GlastOut_full[x]++] = y;
		Gin_full[GlastIn_full[y]++] = x;
	}
	
	double avg_common =0, avg_a_covered = 0, avg_b_covered = 0, avg_sym_diff = 0.0;
	int main_iterations = 0, a_reachable_total = 0, b_reachable_total = 0, common_reachable_total = 0, sym_diff_total = 0;
	while(main_iterations < NUM_TO_SELECT+1){
		for(int i = 0; i<=n; i++) new_reachable_created_a[i] = new_reachable_created_b[i] = 0;
		
		int total_cascade_size_a = 0;
		int total_cascade_size_b = 0;
		a_reachable_total = b_reachable_total = common_reachable_total = sym_diff_total =0;
		for (int i=0; i<K; i++){
			srand(rand_seeds[i]);
			int counter_a = 0;
			int counter_b = 0;
			
			edges_sampled.clear();
	      
			for (unsigned j=0; j<InputEdges.size(); j+=2){
				if((((double)rand())/((double)(RAND_MAX)) ) < EdgeProb_a.at(j/2)){
					counter_a++;
					edges_sampled.push_back(InputEdges.at(j));
					edges_sampled.push_back(InputEdges.at(j+1));
					//printf("(%d,%d) exists\n",InputEdges.at(j),InputEdges.at(j+1));
				}
				//x = InputEdges.at(i);
				//y = InputEdges.at(i+1);
			}
			
			
			// Constructs the reverse graph...
			//printf("\n\n");
			GlastOut_a[0] = 0;//GlastIn[0] = 0;
			//GlastOut[0] = GlastIn[0] = 0;
			
			for (int j = 0; j <= n+1; j++){
				GfirstOut_a[j] = 0;
			}
			
			for (unsigned j=0; j<edges_sampled.size(); j+=2){
				y = edges_sampled.at(j);
				x = edges_sampled.at(j+1);
				GfirstOut_a[x+1]++;
			}
			
			for (int k = 1; k <= n+1; k++) {
				GfirstOut_a[k] += GfirstOut_a[k-1]; // the +1 is to be able to add edges in the recidual
				GlastOut_a[k] = GfirstOut_a[k];
			}
			
			for (unsigned j=0; j<edges_sampled.size(); j+=2){
				y = edges_sampled.at(j);
				x = edges_sampled.at(j+1);
				Gout_a[GlastOut_a[x]++] = y;
			}
	      
			for (int j = 0; j <= n+1; j++) reachable_a[j] = reachable_cascade_a[j] = 0;
		  	int reachable_num_a = reachable_from_initial_seeds(GfirstOut_a, Gout_a, reachable_cascade_a, reachable_a, a_seeds);
		
	
			// =========== do the same for the cascades from the second campaign
		
			edges_sampled.clear();
	      
			for (unsigned j=0; j<InputEdges.size(); j+=2){
				if((((double)rand())/((double)(RAND_MAX)) ) < EdgeProb_b.at(j/2)){
					counter_b++;
					edges_sampled.push_back(InputEdges.at(j));
					edges_sampled.push_back(InputEdges.at(j+1));
				}
				//x = InputEdges.at(i);
				//y = InputEdges.at(i+1);
			}
			
			GlastOut_b[0] = 0;//GlastIn[0] = 0;
			//GlastOut[0] = GlastIn[0] = 0;
			
			for (int j = 0; j <= n+1; j++){
				GfirstOut_b[j] = 0;
			}
			
			for (unsigned j=0; j<edges_sampled.size(); j+=2){
				y = edges_sampled.at(j);
				x = edges_sampled.at(j+1);
				GfirstOut_b[x+1]++;
				//GfirstIn[y+1]++;
			}
			
			for (int k = 1; k <= n+1; k++) {
				GfirstOut_b[k] += GfirstOut_b[k-1]; // the +1 is to be able to add edges in the recidual
				GlastOut_b[k] = GfirstOut_b[k];
				//GfirstIn[k] += GfirstIn[k-1];
				//GlastIn[k] = GfirstIn[k];
			}
	
			for(unsigned j=0; j<edges_sampled.size(); j+=2){
				y = edges_sampled.at(j);
				x = edges_sampled.at(j+1);
				Gout_b[GlastOut_b[x]++] = y;
				//Gin[GlastIn[y]++] = x;
			}
	      
			
			for (int j = 0; j <= n+1; j++) reachable_b[j] = reachable_cascade_b[j] = 0;
			int reachable_num_b = reachable_from_initial_seeds(GfirstOut_b, Gout_b, reachable_cascade_b, reachable_b, b_seeds);
			
			int common_reachable = 0;
			for(int j = 1; j<=n ;j++){
				if(reachable_a[j]){
					a_reachable_total++;
					sym_diff_total++;
				}
				if(reachable_b[j]){
					b_reachable_total++;
					sym_diff_total++;
				}
				if(reachable_a[j] && reachable_b[j]){
				  common_reachable++;
				  common_reachable_total++;
				  sym_diff_total -=2;
				}
			}
			
			int reachable_num;
			for(int j = 1; j<=n ;j++){
				if(!reachable_cascade_a[j]){
					reachable_num = reachable_from_vertex(j, GfirstOut_a, Gout_a, reachable_cascade_a, reachable_a, reachable_b);
					new_reachable_created_a[j]+= reachable_num; 
					//cout << "vertex "<< j <<" in a balances " << reachable_num << " vertices" << endl;
				}
				if(!reachable_cascade_b[j]){
					reachable_num = reachable_from_vertex(j, GfirstOut_b, Gout_b, reachable_cascade_b, reachable_b, reachable_a);
					new_reachable_created_b[j]+= reachable_num;
					//cout << "vertex "<< j <<" in b balances " << reachable_num << "vertices" << endl;
				}
			}
			//cout << i <<"-th sampling contains " << counter_a << " ("<< reachable_num_a <<" reachable vertices) and " << counter_b << " ("<< reachable_num_b <<" reachable vertices) edges , common reachable: "<<common_reachable<<" , respectively "<< endl;
			//cout << "best vertex reaches " << max_reachable_num << "vertices" << endl;
			
			avg_a_covered = ((double)a_reachable_total)/((double)(i+1));
			avg_b_covered = ((double)b_reachable_total)/((double)(i+1));
			avg_common = ((double)common_reachable_total)/((double)(i+1));
			avg_sym_diff = ((double)sym_diff_total)/((double)(i+1));
			
			total_cascade_size_a += counter_a;
			total_cascade_size_b += counter_b;
			if(main_iterations != NUM_TO_SELECT && (i+1)%100 == 0)cout << "Selecting "<< main_iterations+1<<"-th seed ("<< (i+1)<<"-th simulation): average number covered by a:" <<avg_a_covered <<", average covered by b:" <<avg_b_covered<<", average common covered:"<<avg_common<<", average symmetric difference:"<<avg_sym_diff<<endl;
			if(main_iterations == NUM_TO_SELECT && (i+1)%100 == 0) cout << "Evaluating solution: average number covered by a:" <<avg_a_covered <<", average covered by b:" <<avg_b_covered<<", average common covered:"<<avg_common<<", average symmetric difference:"<<avg_sym_diff<< ", number of balanced vertices: " << n-avg_sym_diff << endl;
			if(main_iterations == NUM_TO_SELECT && (i+1) == 1000) cout << "Average cascade a size: " << (double)total_cascade_size_a / (double) (i+1) <<", average cascade b size: " << (double)total_cascade_size_b / (double) (i+1) << endl;
		}
		
		if(main_iterations == NUM_TO_SELECT) break;
		int best_value_a=0, best_vertex_a, best_value_b=0, best_vertex_b;
		for(int j = 1; j<=n ;j++){
			//cout << "vertex "<< j <<" in team a will balance "<< (double)new_reachable_created_a[j]/(double)K <<" vertices on average"<< endl;
			//cout << "vertex "<< j <<" in team b will balance "<< (double)new_reachable_created_b[j]/(double)K <<" vertices on average"<< endl;
			if(new_reachable_created_a[j] > best_value_a){
				best_value_a = new_reachable_created_a[j];
				best_vertex_a = j;
			} 
			if(new_reachable_created_b[j] > best_value_b){
				best_value_b = new_reachable_created_b[j];
				best_vertex_b = j;
			} 
		}
		
		if(best_value_a == 0 && best_value_b == 0) {
			cout << "any possible selection does not improve the objective function" << endl;
			main_iterations = NUM_TO_SELECT;
			continue;
		}
		if(best_value_a > best_value_b){
			for (map<string,int>::iterator it = user2ID.begin(); it != user2ID.end(); ++it ){
				if (it->second == best_vertex_a){
					cout << "--> seed  "<< it->first <<"  is selected in the first campaign and balances "<< (double)best_value_a/(double)K <<" vertices on average"<< endl;
				}
			}
			a_seeds.push_back(best_vertex_a);
			main_iterations+=1;
		}
		else{
			for (map<string,int>::iterator it = user2ID.begin(); it != user2ID.end(); ++it ){
				if (it->second == best_vertex_b){
					cout << "--> seed  "<< it->first <<"  is selected in the second campaign and balances "<< (double)best_value_a/(double)K <<" vertices on average"<< endl;
				}
			}
			//cout << "vertex "<< best_vertex_b <<" in team b will balance "<< (double)best_value_b/(double)K <<" vertices on average"<< endl;
			b_seeds.push_back(best_vertex_b);
			main_iterations+=1;
		}
	}
}

/* read graph from input file */
void readGraph(const char *file,const char *file2, const char *file3) {
//     FILE *input = fopen(file, "r");
//     if (!input) {
//         fprintf(stderr, "Error opening file \"%s\".\n", file);
//         exit(-1);
//     }
    
    int x, y;
    int p = 0;
    int count_edges = 0;
    int count_vertices= 1;
    string user_a, user_b;
    long double prob_a,prob_b;
    
    map<string,int>::iterator it;
    
    ifstream infile(file);
	while (infile >> user_a >> user_b >> prob_a >> prob_b){
		it = user2ID.find(user_a);
		if(it != user2ID.end()) x = it->second;
		else{
		    user2ID[user_a]=count_vertices;
		    x = count_vertices;
		    count_vertices++;
		}
		
		it = user2ID.find(user_b);
		if(it != user2ID.end()) y = it->second;
		else{
		    user2ID[user_b]=count_vertices;
		    y = count_vertices;
		    count_vertices++;
		}
		count_edges++;
		InputEdges.push_back(x);
		InputEdges.push_back(y);
		//printf("(%d,%d)\n",x,y);
		EdgeProb_a.push_back(prob_a);
		EdgeProb_b.push_back(prob_b);
		// process pair (a,b)
	}
	n = count_vertices-1;
	m = count_edges;
	
    cout << "distinct users:" << n << " number of edges:" << count_edges << endl;
    
    ifstream file_seeds_a(file2);
    while (file_seeds_a >> user_a){
		it = user2ID.find(user_a);
		if(it != user2ID.end()) a_seeds.push_back(it->second);
		else{
		    cout << "Error a : seed " << user_a << " is not part of the input graph" << endl;
		    //exit(1);
		}
    }
    while(a_seeds.size() > 40) a_seeds.pop_back();
    ifstream file_seeds_b(file3);
	while (file_seeds_b >> user_b){
		it = user2ID.find(user_b);
		if(it != user2ID.end()) b_seeds.push_back(it->second);
		else{
		    cout << "Error b : seed " << user_a << " is not part of the input graph" << endl;
		    //exit(1);
		}
	}
    while(b_seeds.size() > 40) b_seeds.pop_back();
    
    /*
    while (fgets(line, MAXLINE, input) != NULL) {
      printf("users: %d\n",count_vertices);
        if (sscanf(line, "%s\t%s\t%Lf\t%Lf",&user_a, &user_b, &prob_a, &prob_b) != 4) {
	    fprintf(stderr, "Error reading graph size (%s).\n", file);
	    exit(-1);
	}
	
	printf("%s\n",user_a);
	
	if(user2ID.find(user_a) == user2ID.end()){
	    user2ID.insert(std::pair<string,int>(user_a,count_vertices) );
	    count_vertices++;
	}
	if(user2ID.find(user_b)== user2ID.end()){
	    user2ID.insert(std::pair<string,int>(user_b,count_vertices) );
	    count_vertices++;
	}
    }*/
    if(count_edges<m) m = count_edges;

    fprintf(stderr, "END reading graph (%s).\n", file);
//     fclose(input);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("\n usage: %s <input file> <seeds 1 file> <seeds 2 file><numseeds>\n\n", argv[0]);
        exit(-1);
    }

    char *file = argv[1];
    char *file2 = argv[2];
    char *file3 = argv[3];
    int NUM_TO_SELECT = atoi(argv[4]);

    readGraph(file, file2, file3);

    RFWTimer timer(true);
    double t;

    srand(time(NULL));
//     starting_vertex = 1+(int)(rand() % n);
//     processInput();
	Greedy(NUM_TO_SELECT);

    t = timer.getTime();
    fprintf (stderr, "totaltime %f\n", t);

    return 0;
}
