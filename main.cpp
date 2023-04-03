#include <iostream>
#include "graph.h"
#include <metis.h>
#define original 1
#include <fstream>
using namespace std;
#include <set>
#include <vector>
#include <algorithm>
#include <numeric>
#define verify_split 1
// #define print_subgraphs 1
struct graph_t
{
	long* begin_pos;
	long* csr;
	long vert_count;
	long edge_count;
	graph_t(long nvtx, long nnz)
	{
		vert_count = nvtx;
		edge_count = 0;//incremented gradually while filling the csr for the graph.
		begin_pos = (long*)malloc((nvtx+1)*sizeof(long));
		csr = (long*)malloc(nnz*sizeof(long));
	}
	graph_t()
	{
		// vert_count = nvtx;
		edge_count = 0;//incremented gradually while filling the csr for the graph.
		begin_pos = nullptr;
		csr = nullptr;
	}
	// ~graph_t()
	// {
	// 	free(begin_pos);
	// 	free(csr);
	// }
};

int Split_Graph(long* beg_pos,long* csr, long vert_count, long* partition, std::vector<struct graph_t>& subgraphs)
{
	//count the number of subgraphs
	int Nsubgraphs = 0;
	std::set<int> Nsubgraph_set;
	long* mapping = new long[vert_count];	
	for(int i = 0; i < vert_count; i++)
	{
		//insert into the set
		Nsubgraph_set.insert(partition[i]);
	}
	Nsubgraphs = Nsubgraph_set.size();
	cout<<"Number of partitions/subgraph detected: "<<Nsubgraphs<<endl;
	//Find number of vertices and edges in each subgraph
	long* Nvertices = new long[Nsubgraphs];
	long* Nedges = new long[Nsubgraphs];
	memset(Nvertices, 0, Nsubgraphs*sizeof(long));
	memset(Nedges, 0, Nsubgraphs*sizeof(long));	
	for(long i = 0; i < vert_count; i++)
	{
		long part = partition[i];
		mapping[i] = Nvertices[part];
		// cout<<"Vertex: "<<i<<" is mapped to: "<<mapping[i]<<" in subgraph: "<<part<<endl;
		Nvertices[part]++;
		//count the edges
		for(long j = beg_pos[i]; j < beg_pos[i+1]; j++)
		{
			long neighbor = csr[j];
			if(partition[neighbor] == part)
				Nedges[part]++;			
		}
	}
	//Allocate memory for each subgraph
	// struct graph_t* subgraphs = new graph_t[Nsubgraphs];
	for(int i = 0; i < Nsubgraphs; i++)
	{
		// cout<<"Allocating subgraph: "<<i<<endl;		
		graph_t subgraph(Nvertices[i], Nedges[i]);
		subgraphs.push_back(subgraph);	
		// cout<<"Allocated subgraph "<<i<<" with "<<Nvertices[i]<<" vertices and "<<Nedges[i]<<" edges"<<endl;		
	}
	//Fill in the subgraphs
	for(long i = 0; i < vert_count; i++)
	{
		long part = partition[i];
		long new_i = mapping[i];
		graph_t* subgraph = &subgraphs[part];
		fflush(stdout);
		subgraph->begin_pos[new_i] = subgraph->edge_count;
		for(long j = beg_pos[i]; j < beg_pos[i+1]; j++)
		{
			long neighbor = csr[j];
			if(partition[neighbor] == part)
			{
				subgraph->csr[subgraph->edge_count] = mapping[neighbor];
				subgraph->edge_count++;		
			}
		}		
	}
	//Fill in the last element of begin_pos
	for(int i = 0; i < Nsubgraphs; i++)
	{
		subgraphs[i].begin_pos[Nvertices[i]] = subgraphs[i].edge_count;
	}
	//Sort the neighbors of each vertex in each subgraph	
	for(int i = 0; i < Nsubgraphs; i++)
	{		
		graph_t* subgraph = &subgraphs[i];
		for(long j = 0; j < Nvertices[i]; j++)
		{
			long beg = subgraph->begin_pos[j];
			long end = subgraph->begin_pos[j+1];
			std::sort(subgraph->csr+beg, subgraph->csr+end);
		}
	}
	#ifdef verify_split
	//Verify the split
	//verifying Sum of vertex count each of the subgraphs equals to the vertex count in orginal graph
	assert(vert_count == std::accumulate(Nvertices, Nvertices+Nsubgraphs, 0));
	for(long i = 0; i < vert_count; i++)
	{
		long part = partition[i];
		graph_t subgraph = subgraphs[part];
		long new_i = mapping[i];
		for(long j = beg_pos[i]; j < beg_pos[i+1]; j++)
		{
			long neighbor = csr[j];
			long new_neighbor = mapping[neighbor];
			long part_neighbor = partition[neighbor];
			if(part_neighbor == part)
			{
				//check if the edge is present in the subgraph
				//Verifying same partitions neighbours of vertices are in same subgraph.
				long beg = subgraph.begin_pos[new_i];
				long end = subgraph.begin_pos[new_i+1];
				long* beg_ptr = std::lower_bound(subgraph.csr+beg, subgraph.csr+end, new_neighbor);// Assumes the adjacency list is sorted.
				if(beg_ptr == subgraph.csr+end)
				{
					std::cout<<"Error in split\n";
					return -1;
				}
			}
		}
	}
	cout<<"Split verified! Following tests are done:-\n";
	cout<<"\t 1: "<<Nsubgraphs<<" subgraphs created!"<<endl;
	cout<<"\t 2: "<<"Sum of vertex count each of the subgraphs equals to the vertex count in orginal graph!"<<endl;
	cout<<"\t 3: "<<"Verified same partitions neighbours of vertices are in same subgraph!"<<endl;
	#endif
	// *subgraphs1 = subgraphs;
	delete[] mapping;
	return Nsubgraphs;
}

int main(int args, char **argv)
{
	std::cout<<"Input: ./exe beg csr weight\n";
	if(args!=4){std::cout<<"Wrong input\n"; return -1;}
	
	const char *beg_file=argv[1];
	const char *csr_file=argv[2];
	const char *weight_file=argv[3];
	
	//template <file_vertex_t, file_index_t, file_weight_t
	//new_vertex_t, new_index_t, new_weight_t>
	graph<long, long, int, long, long, char>
	*ginst = new graph
	<long, long, int, long, long, char>
	(beg_file,csr_file,weight_file);
	int64_t npartitions = 2;
	int64_t objval = 1;	
	int64_t nbc = 1;
	long int* result =  (long int *) malloc(ginst->vert_count * sizeof(long int));

#ifdef original
	int status = METIS_PartGraphKway(&ginst->vert_count,&nbc,ginst->beg_pos,ginst->csr, NULL, NULL,NULL,
			  &npartitions, NULL, NULL,NULL,&objval,
			  result);    
#else
int status = METIS_PartGraphRecursive(&ginst->vert_count,&nbc,ginst->beg_pos,ginst->csr, NULL, NULL,NULL,
                            &npartitions, NULL, NULL,NULL,&objval,
                            result);    
#endif
	if (status == METIS_OK) {
	    printf("Partitioning successful. Edgecut: %d\n", objval);
	    /* ... use the partitioning stored in 'part' ... */
	} else {
	    printf("Error during partitioning.\n");
	}
	// struct graph_t* subgraphs;
	std::vector<struct graph_t> subgraphs;
	//output graphs in subgraphs structure.		
	int Nsubgraphs = Split_Graph(ginst->beg_pos, ginst->csr, ginst->vert_count,	result,	 subgraphs);

#ifdef print_subgraphs
	cout<<"Partition setting after METIS: "<<endl;
	for(int i = 0; i < ginst->vert_count; i++)
	{
		cout<<result[i]<<" ";
	}
	cout<<endl;
	cout<<"Number of subgraphs created: "<<Nsubgraphs<<endl;

    ofstream outfile("output.txt");
    
    if (outfile.is_open()) {
        for (long int i = 0; i < ginst->vert_count; i++) {
            outfile << result[i] << " ";
        }
        outfile.close();
        cout << "partition has been written to file successfully." << endl;
    } else {
        cout << "Unable to open file." << endl;
    }

	//printing the subgraphs
	for(int i = 0; i < Nsubgraphs; i++)
	{
		cout<<"Subgraph "<<i<<endl;
		graph_t graph = subgraphs[i];
		cout<<"Number of vertices: "<<graph.vert_count<<endl;
		cout<<"Number of edges: "<<graph.edge_count<<endl;
		
		for(int j = 0; j < graph.vert_count; j++)
		{
			long beg = graph.begin_pos[j];
			long end = graph.begin_pos[j+1];
			cout<<j<<"'s neighor list: ";
			for (long k =beg; k < end; k++)
			{
				cout<<graph.csr[k]<<" ";
			}
			cout<<endl;
		}
		cout<<endl<<endl;
	}

	cout<<"Printing original graph"<<endl;
	cout<<"Number of vertices: "<<ginst->vert_count<<endl;
	cout<<"Number of edges: "<<ginst->edge_count<<endl;
	for (int i = 0; i < ginst->vert_count; i++)
	{
		long beg = ginst->beg_pos[i];
		long end = ginst->beg_pos[i+1];
		cout<<i<<"'s neighor list: ";
		for (long j = beg; j < end; j++)
		{
			// cout<<ginst->csr[j]<<"(P"<<result[ginst->csr[j]]<<")(M"<<mapping[ginst->csr[j]]<<")"<<" ";
			cout<<ginst->csr[j]<<"(P"<<result[ginst->csr[j]]<<") ";
		}
		cout<<endl;
	}
	cout<<endl<<endl;
#endif
	return 0;	
}
