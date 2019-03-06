// Writing some basic tests for CUDA kernels.

#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <cstring>

#include "cudapoa_kernels.cuh"

void cudaCheckError(cudaError_t error, const char* msg, uint32_t line, const char* file)
{
    if (error != cudaSuccess)
    {
        std::cerr << msg << " (" << cudaGetErrorString(error) << ") " << \
            "on " << file << ":" << line << std::endl;
        exit(-1);
    }
}

void testTopologicalSort_1()
{
    std::cout << "Running " << __func__ << std::endl;
    /*
             |----->F
             |
             |
       E---->A----->B----->D
                    ^
                    |
             C------|
    */
    uint32_t num_nodes = 6;
    uint8_t* nodes = new uint8_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted_node_map = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    nodes[0] = 'A';
    nodes[1] = 'B';
    nodes[2] = 'C';
    nodes[3] = 'D';
    nodes[4] = 'E';
    nodes[5] = 'F';

    cudaError_t error = cudaSuccess;

    // Allocate device buffer for results.
    uint16_t* sorted_d;
    error = cudaMalloc((void**) &sorted_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    uint16_t* sorted_node_map_d;
    error = cudaMalloc((void**) &sorted_node_map_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate host and device buufer for incoming edge counts.
    uint16_t* incoming_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    incoming_edge_count[0] = 1; 
    incoming_edge_count[1] = 2;
    incoming_edge_count[2] = 0;
    incoming_edge_count[3] = 1;
    incoming_edge_count[4] = 0;
    incoming_edge_count[5] = 1;
    uint16_t* incoming_edge_count_d;
    error = cudaMalloc((void**) &incoming_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(incoming_edge_count_d, incoming_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edges.
    uint16_t* outgoing_edges = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES];
    outgoing_edges[0 * CUDAPOA_MAX_NODE_EDGES + 0] = 5;
    outgoing_edges[0 * CUDAPOA_MAX_NODE_EDGES + 1] = 1;
    outgoing_edges[1 * CUDAPOA_MAX_NODE_EDGES + 0] = 3;
    outgoing_edges[2 * CUDAPOA_MAX_NODE_EDGES + 0] = 1;
    outgoing_edges[4 * CUDAPOA_MAX_NODE_EDGES + 0] = 0;
    uint16_t* outgoing_edges_d;
    error = cudaMalloc((void**) &outgoing_edges_d, 
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edges_d, outgoing_edges,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edge count.
    uint16_t* outgoing_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    outgoing_edge_count[0] = 2;
    outgoing_edge_count[1] = 1;
    outgoing_edge_count[2] = 1;
    outgoing_edge_count[4] = 1;
    uint16_t* outgoing_edge_count_d;
    error = cudaMalloc((void**) &outgoing_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edge_count_d, outgoing_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Run topological sort.
    nvidia::cudapoa::topologicalSort(sorted_d, sorted_node_map_d, num_nodes, incoming_edge_count_d,
                    outgoing_edges_d, outgoing_edge_count_d,
                    1, 1, 0);
    error = cudaDeviceSynchronize();
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Copy results back to memory and print them.
    error = cudaMemcpy(sorted, sorted_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(sorted_node_map, sorted_node_map_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Verify results.
    assert (sorted[0] == 2); // C
    assert (sorted[1] == 4); // E
    assert (sorted[2] == 0); // A
    assert (sorted[3] == 5); // F
    assert (sorted[4] == 1); // B
    assert (sorted[5] == 3); // D

    assert (sorted_node_map[0] == 2); // C
    assert (sorted_node_map[1] == 4); // E
    assert (sorted_node_map[2] == 0); // A
    assert (sorted_node_map[3] == 5); // F
    assert (sorted_node_map[4] == 1); // B
    assert (sorted_node_map[5] == 3); // D

    // Uncomment to print final sorted list.
    for(uint32_t i = 0; i < num_nodes; i++)
    {
        std::cout << nodes[sorted[i]] << ", " << std::endl;
    }
}

void testNW_1()
{
    std::cout << "Running " << __func__ << std::endl;
    /*
             |----->F------|
             |             |
             |             \/
       E---->A----->B----->D
                    ^
                    |
             C------|
    */
    uint32_t num_nodes = 6;
    uint8_t* nodes = new uint8_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted_node_map = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    nodes[0] = 'A';
    nodes[1] = 'B';
    nodes[2] = 'C';
    nodes[3] = 'D';
    nodes[4] = 'E';
    nodes[5] = 'F';

    cudaError_t error = cudaSuccess;
    uint8_t* nodes_d;
    error = cudaMalloc((void**) &nodes_d,
                       sizeof(uint8_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);

    error = cudaMemcpy(nodes_d, nodes, sizeof(uint8_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
            cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device buffer for results.
    uint16_t* sorted_d;
    error = cudaMalloc((void**) &sorted_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    uint16_t* sorted_node_map_d;
    error = cudaMalloc((void**) &sorted_node_map_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for incoming edges.
    uint16_t* incoming_edges = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES];
    incoming_edges[0 * CUDAPOA_MAX_NODE_EDGES + 0] = 4;
    incoming_edges[1 * CUDAPOA_MAX_NODE_EDGES + 0] = 0;
    incoming_edges[1 * CUDAPOA_MAX_NODE_EDGES + 1] = 2;
    incoming_edges[3 * CUDAPOA_MAX_NODE_EDGES + 0] = 1;
    incoming_edges[3 * CUDAPOA_MAX_NODE_EDGES + 1] = 5;
    incoming_edges[5 * CUDAPOA_MAX_NODE_EDGES + 0] = 0;
    uint16_t* incoming_edges_d;
    error = cudaMalloc((void**) &incoming_edges_d, 
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(incoming_edges_d, incoming_edges,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate host and device buufer for incoming edge counts.
    uint16_t* incoming_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    incoming_edge_count[0] = 1; 
    incoming_edge_count[1] = 2;
    incoming_edge_count[2] = 0;
    incoming_edge_count[3] = 2;
    incoming_edge_count[4] = 0;
    incoming_edge_count[5] = 1;
    uint16_t* incoming_edge_count_d;
    error = cudaMalloc((void**) &incoming_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(incoming_edge_count_d, incoming_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edges.
    uint16_t* outgoing_edges = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES];
    outgoing_edges[0 * CUDAPOA_MAX_NODE_EDGES + 0] = 5;
    outgoing_edges[0 * CUDAPOA_MAX_NODE_EDGES + 1] = 1;
    outgoing_edges[1 * CUDAPOA_MAX_NODE_EDGES + 0] = 3;
    outgoing_edges[2 * CUDAPOA_MAX_NODE_EDGES + 0] = 1;
    outgoing_edges[4 * CUDAPOA_MAX_NODE_EDGES + 0] = 0;
    outgoing_edges[5 * CUDAPOA_MAX_NODE_EDGES + 0] = 3;
    uint16_t* outgoing_edges_d;
    error = cudaMalloc((void**) &outgoing_edges_d, 
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edges_d, outgoing_edges,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edge count.
    uint16_t* outgoing_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    outgoing_edge_count[0] = 2;
    outgoing_edge_count[1] = 1;
    outgoing_edge_count[2] = 1;
    outgoing_edge_count[4] = 1;
    outgoing_edge_count[5] = 1;
    uint16_t* outgoing_edge_count_d;
    error = cudaMalloc((void**) &outgoing_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edge_count_d, outgoing_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Run topological sort.
    nvidia::cudapoa::topologicalSort(sorted_d, sorted_node_map_d, num_nodes, incoming_edge_count_d,
                    outgoing_edges_d, outgoing_edge_count_d,
                    1, 1, 0);
    error = cudaDeviceSynchronize();
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Copy results back to memory and print them.
    error = cudaMemcpy(sorted, sorted_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(sorted_node_map, sorted_node_map_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);


    // Verify results.
    assert (sorted[0] == 2); // C
    assert (sorted[1] == 4); // E
    assert (sorted[2] == 0); // A
    assert (sorted[3] == 5); // F
    assert (sorted[4] == 1); // B
    assert (sorted[5] == 3); // D

    assert (sorted_node_map[0] == 2); // C
    assert (sorted_node_map[1] == 4); // E
    assert (sorted_node_map[2] == 0); // A
    assert (sorted_node_map[3] == 5); // F
    assert (sorted_node_map[4] == 1); // B
    assert (sorted_node_map[5] == 3); // D

    // Uncomment to print final sorted list.
    for(uint32_t i = 0; i < num_nodes; i++)
    {
        std::cout << nodes[sorted[i]] << ", " << std::endl;
    }

    const int32_t read_count = 4;
    uint8_t read[read_count];
    read[0] = 'E';
    read[1] = 'G';
    read[2] = 'F';
    read[3] = 'D';
    uint8_t *read_d;
    cudaMalloc((void**) &read_d, sizeof(uint8_t) * read_count);
    cudaMemcpy(read_d, read, read_count, cudaMemcpyHostToDevice);

    int32_t* scores_d;
    int16_t *traceback_i_d, *traceback_j_d;
    cudaMalloc((void**) &scores_d, sizeof(int32_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));
    cudaMalloc((void**) &traceback_i_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));
    cudaMalloc((void**) &traceback_j_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));

    // Run needleman wunsch.
    nvidia::cudapoa::needlemanWunsch(nodes_d, sorted_d, sorted_node_map_d, num_nodes, 
                                     incoming_edge_count_d,incoming_edges_d, 
                                     outgoing_edge_count_d,outgoing_edges_d, 
                                     read_d, read_count,
                                     scores_d, traceback_i_d, traceback_j_d,
                                     1, 1, 0);

    error = cudaDeviceSynchronize();
    int16_t* traceback_i = new int16_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];
    int16_t* traceback_j = new int16_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];
    int32_t* scores = new int32_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];

    cudaMemcpy(traceback_i, traceback_i_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);
    cudaMemcpy(traceback_j, traceback_j_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);
    cudaMemcpy(scores, scores_d, sizeof(int32_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);

    printf("host results\n");
    for(uint16_t i = 0; i < 10; i++)
    {
        int16_t node_id = traceback_i[i];
        int16_t read_pos = traceback_j[i];
        printf("(%c, %c)\n", (node_id == -1 ? '-' : nodes[node_id]),
                             (read_pos == -1 ? '-' : read[read_pos]));
    }

}

void testAddAlignment_1()
{
    std::cout << "Running " << __func__ << std::endl;
    /*
       A---->B----->C----->D----->E
    */
    uint32_t num_nodes = 5;
    uint8_t* nodes = new uint8_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    uint16_t* sorted_node_map = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    nodes[0] = 'A';
    nodes[1] = 'B';
    nodes[2] = 'C';
    nodes[3] = 'D';
    nodes[4] = 'E';

    cudaError_t error = cudaSuccess;
    uint8_t* nodes_d;
    error = cudaMalloc((void**) &nodes_d,
                       sizeof(uint8_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);

    error = cudaMemcpy(nodes_d, nodes, sizeof(uint8_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
            cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device buffer for results.
    uint16_t* sorted_d;
    error = cudaMalloc((void**) &sorted_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    uint16_t* sorted_node_map_d;
    error = cudaMalloc((void**) &sorted_node_map_d,
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for incoming edges.
    uint16_t* incoming_edges = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES];
    incoming_edges[1 * CUDAPOA_MAX_NODE_EDGES + 0] = 0;
    incoming_edges[2 * CUDAPOA_MAX_NODE_EDGES + 0] = 1;
    incoming_edges[3 * CUDAPOA_MAX_NODE_EDGES + 0] = 2;
    incoming_edges[4 * CUDAPOA_MAX_NODE_EDGES + 0] = 3;
    uint16_t* incoming_edges_d;
    error = cudaMalloc((void**) &incoming_edges_d, 
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(incoming_edges_d, incoming_edges,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate host and device buufer for incoming edge counts.
    uint16_t* incoming_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    incoming_edge_count[0] = 0; 
    incoming_edge_count[1] = 1;
    incoming_edge_count[2] = 1;
    incoming_edge_count[3] = 1;
    incoming_edge_count[4] = 1;
    uint16_t* incoming_edge_count_d;
    error = cudaMalloc((void**) &incoming_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(incoming_edge_count_d, incoming_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edges.
    uint16_t* outgoing_edges = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES];
    outgoing_edges[0 * CUDAPOA_MAX_NODE_EDGES + 0] = 1;
    outgoing_edges[1 * CUDAPOA_MAX_NODE_EDGES + 0] = 2;
    outgoing_edges[2 * CUDAPOA_MAX_NODE_EDGES + 0] = 3;
    outgoing_edges[3 * CUDAPOA_MAX_NODE_EDGES + 0] = 4;
    uint16_t* outgoing_edges_d;
    error = cudaMalloc((void**) &outgoing_edges_d, 
                       sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edges_d, outgoing_edges,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_EDGES,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffer for outgoing edge count.
    uint16_t* outgoing_edge_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    outgoing_edge_count[0] = 1;
    outgoing_edge_count[1] = 1;
    outgoing_edge_count[2] = 1;
    outgoing_edge_count[3] = 1;
    outgoing_edge_count[4] = 0;
    uint16_t* outgoing_edge_count_d;
    error = cudaMalloc((void**) &outgoing_edge_count_d,
                        sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(outgoing_edge_count_d, outgoing_edge_count,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Allocate device and host buffers for aligned nodes count.
    uint16_t* node_alignments = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_ALIGNMENTS];
    memset(node_alignments, 0, sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_ALIGNMENTS);
    uint16_t* node_alignments_d;
    cudaMalloc((void**) &node_alignments_d,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_ALIGNMENTS);
    error = cudaMemcpy(node_alignments_d, node_alignments,
            sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW * CUDAPOA_MAX_NODE_ALIGNMENTS,
            cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    uint16_t* node_alignment_count = new uint16_t[CUDAPOA_MAX_NODES_PER_WINDOW];
    memset(node_alignment_count, 0, sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    uint16_t* node_alignment_count_d;
    cudaMalloc((void**) &node_alignment_count_d,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW);
    error = cudaMemcpy(node_alignment_count_d, node_alignment_count,
            sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
            cudaMemcpyHostToDevice);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Run topological sort.
    nvidia::cudapoa::topologicalSort(sorted_d, sorted_node_map_d, num_nodes, incoming_edge_count_d,
                    outgoing_edges_d, outgoing_edge_count_d,
                    1, 1, 0);
    error = cudaDeviceSynchronize();
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Copy results back to memory and print them.
    error = cudaMemcpy(sorted, sorted_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);
    error = cudaMemcpy(sorted_node_map, sorted_node_map_d, sizeof(uint16_t) * num_nodes,
                       cudaMemcpyDeviceToHost);
    cudaCheckError(error, "", __LINE__, __FILE__);

    // Uncomment to print final sorted list.
    for(uint32_t i = 0; i < num_nodes; i++)
    {
        std::cout << nodes[sorted[i]] << ", " << std::endl;
    }

    const int32_t read_count = 5;
    uint8_t read[read_count];
    read[0] = 'G';
    read[1] = 'B';
    read[2] = 'C';
    read[3] = 'F';
    read[4] = 'E';
    uint8_t *read_d;
    cudaMalloc((void**) &read_d, sizeof(uint8_t) * read_count);
    cudaMemcpy(read_d, read, read_count, cudaMemcpyHostToDevice);

    int32_t* scores_d;
    int16_t *traceback_i_d, *traceback_j_d;
    cudaMalloc((void**) &scores_d, sizeof(int32_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));
    cudaMalloc((void**) &traceback_i_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));
    cudaMalloc((void**) &traceback_j_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1));

    // Run needleman wunsch.
    nvidia::cudapoa::needlemanWunsch(nodes_d, sorted_d, sorted_node_map_d, num_nodes, 
                                     incoming_edge_count_d,incoming_edges_d, 
                                     outgoing_edge_count_d,outgoing_edges_d, 
                                     read_d, read_count,
                                     scores_d, traceback_i_d, traceback_j_d,
                                     1, 1, 0);

    error = cudaDeviceSynchronize();
    int16_t* traceback_i = new int16_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];
    int16_t* traceback_j = new int16_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];
    int32_t* scores = new int32_t[(CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1)];

    cudaMemcpy(traceback_i, traceback_i_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);
    cudaMemcpy(traceback_j, traceback_j_d, sizeof(int16_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);
    cudaMemcpy(scores, scores_d, sizeof(int32_t) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1) * (CUDAPOA_MAX_NODES_PER_WINDOW + 1), cudaMemcpyDeviceToHost);

    printf("host results\n");
    for(uint16_t i = 0; i < 10; i++)
    {
        int16_t node_id = traceback_i[i];
        int16_t read_pos = traceback_j[i];
        printf("(%c, %c)\n", (node_id == -1 ? '-' : nodes[node_id]),
                             (read_pos == -1 ? '-' : read[read_pos]));
    }

    // Run addition of alignment to graph.
    nvidia::cudapoa::addAlignmentToGraphHost(nodes_d, num_nodes,
            node_alignments_d, node_alignment_count_d,
            incoming_edges_d, incoming_edge_count_d,
            outgoing_edges_d,  outgoing_edge_count_d,
            nullptr, nullptr,
            5, 
            sorted_d, traceback_i_d,
            read_d, traceback_j_d,
            0);
    error = cudaDeviceSynchronize();
    error = cudaMemcpy(outgoing_edge_count, outgoing_edge_count_d,
               sizeof(uint16_t) * CUDAPOA_MAX_NODES_PER_WINDOW,
               cudaMemcpyDeviceToHost);
    for(uint32_t i = 0; i < 7; i++)
    {
        printf("%d %d\n", i, outgoing_edge_count[i]);
    }

}

int main()
{
    //testTopologicalSort_1();
    testNW_1();
    testAddAlignment_1();
    return 0;
}
