# split_graph

The code is a simple application to split a graph in CSR format to multiple graphs in CSR format based upto a partition vector of size (V) where V is the number of vertices in the graph.

The code reads a binary CSR dataset "https://github.com/asherliu/graph_project_start/tree/master/tuple_text_to_binary_csr_mem", uses METIS (https://github.com/KarypisLab/METIS) to partition into differnt parts.
The output of METIS is fed to the split_graph module to get a vector of subgraphs in CSR format.
