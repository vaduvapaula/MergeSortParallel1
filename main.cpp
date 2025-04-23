#include <mpi.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>

using namespace std;

vector<int> readFromFile(const string& filename) {
    vector<int> numbers;
    ifstream file(filename);
    int num;
    while (file >> num)
        numbers.push_back(num);
    return numbers;
}

void merge(vector<int>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; ++i) L[i] = arr[left + i];
    for (int i = 0; i < n2; ++i) R[i] = arr[mid + 1 + i];
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

void mergeSort(vector<int>& arr, int left, int right) {
    if (left < right) {
        int m = left + (right - left) / 2;
        mergeSort(arr, left, m);
        mergeSort(arr, m + 1, right);
        merge(arr, left, m, right);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0)
            cerr << "Trebuie sa specifici fisierul de intrare.\n";
        MPI_Finalize();
        return 1;
    }

    string filename = argv[1];
    vector<int> full_data;
    int total_size = 0;

    if (rank == 0) {
        full_data = readFromFile(filename);
        total_size = full_data.size();
    }

    MPI_Bcast(&total_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int local_size = total_size / size;
    int rest = total_size % size;

    vector<int> send_counts(size, local_size);
    vector<int> displs(size, 0);
    for (int i = 0; i < rest; ++i)
        send_counts[i]++;

    for (int i = 1; i < size; ++i)
        displs[i] = displs[i - 1] + send_counts[i - 1];

    vector<int> local_data(send_counts[rank]);

    auto start = chrono::high_resolution_clock::now();

    MPI_Scatterv(full_data.data(), send_counts.data(), displs.data(), MPI_INT,
        local_data.data(), send_counts[rank], MPI_INT,
        0, MPI_COMM_WORLD);

	// local sort
    mergeSort(local_data, 0, local_data.size() - 1);

    vector<int> gathered;
    if (rank == 0)
        gathered.resize(total_size);

    MPI_Gatherv(local_data.data(), send_counts[rank], MPI_INT,
        gathered.data(), send_counts.data(), displs.data(), MPI_INT,
        0, MPI_COMM_WORLD);

    auto end = chrono::high_resolution_clock::now();

    if (rank == 0) {
        // last merge
        int current_size = send_counts[0];
        for (int i = 1; i < size; ++i) {
            merge(gathered, 0, current_size - 1, current_size + send_counts[i] - 1);
            current_size += send_counts[i];
        }

        chrono::duration<double, milli> duration = end - start;
        cout << "Sortare finalizata pentru " << filename << " in " << duration.count() << " ms\n";
    }

    MPI_Finalize();
    return 0;
}
