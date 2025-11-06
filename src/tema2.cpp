#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <bitset>
#include <map>
#include <algorithm>
#include <unordered_map>

using namespace std;


#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define TAG_OWNED_FILES 1
#define TAG_START_DOWNLOAD 2
#define TAG_DOWNLOAD_FILE 3
#define TAG_REQUEST 4
#define TAG_RESPONSE 5
#define TAG_FINISH_DOWNLOAD 6
#define TAG_UPLOAD_CLOSE 7
#define TAG_INCOMING_PEER 8
#define TAG_UPDATE_SWARM 9

// store metada about the files
struct metadata {
    char filename[MAX_FILENAME];
    int num_segments;
    char hashes[MAX_CHUNKS][HASH_SIZE + 1];
    vector<int> swarm;
    bitset<MAX_CHUNKS> owned_segments;
};

struct download_args {
    int rank;
    vector<string> wanted_files;
    vector<metadata> owned_files;
    int end;
};

struct upload_args {
    int rank;
    vector<metadata> owned_files;
    int end;
};

struct sworm {
    int sworm_size;
    unordered_map<string, metadata> details_wanted_files;
    
};

void send_metadata(const metadata& m, int destination, MPI_Comm comm) {
    int swarm_size = m.swarm.size();
    MPI_Send(&swarm_size, 1, MPI_INT, destination, TAG_DOWNLOAD_FILE, comm);
    MPI_Send(m.swarm.data(), swarm_size, MPI_INT, destination, TAG_DOWNLOAD_FILE, comm);
    
}

void send_request(const string& filename, int segment, int peer, MPI_Comm comm) {
    MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, peer, TAG_REQUEST, comm);
    MPI_Send(&segment, 1, MPI_INT, peer, TAG_REQUEST, comm);
}

string receive_response(int peer, MPI_Comm comm) {
    char response[5];
    MPI_Recv(response, 4, MPI_CHAR, peer, TAG_RESPONSE, comm, MPI_STATUS_IGNORE);
    response[4] = '\0';
    return string(response);
}

void receive_swarm(int& swarm_size, vector<int>& swarm, int rank, MPI_Comm comm) {
    MPI_Recv(&swarm_size, 1, MPI_INT, rank, TAG_DOWNLOAD_FILE, comm, MPI_STATUS_IGNORE);

    swarm.resize(swarm_size);

    MPI_Recv(swarm.data(), swarm_size, MPI_INT, rank, TAG_DOWNLOAD_FILE, comm, MPI_STATUS_IGNORE);
}


string segment_response(int peer, metadata& m, int s) {
    char segment[HASH_SIZE + 1];
    MPI_Recv(segment, HASH_SIZE + 1, MPI_CHAR, peer, TAG_RESPONSE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return segment;
}


void *download_thread_func(void *arg)
{
    download_args *args = (download_args*) arg;
    
    unordered_map<string, metadata> details_wanted_files;

    for (const string& filename : args->wanted_files) {
        // ask the tracker for the file
        char filename_c[MAX_FILENAME];
        strcpy(filename_c, filename.c_str());
        MPI_Send(filename_c, MAX_FILENAME, MPI_CHAR, TRACKER_RANK, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD);


        details_wanted_files[filename] = metadata();
        // receive the swarm
        int swarm_size;

        receive_swarm(swarm_size, details_wanted_files[filename].swarm, TRACKER_RANK, MPI_COMM_WORLD);

        // read the number of segments
        MPI_Recv(&details_wanted_files[filename].num_segments, 1, MPI_INT, TRACKER_RANK, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // read the hashes
        for (int j = 0; j < details_wanted_files[filename].num_segments; ++j) {
            MPI_Recv(details_wanted_files[filename].hashes[j], HASH_SIZE + 1, MPI_CHAR, TRACKER_RANK, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // set all the segments as not owned
        for (int j = 0; j < details_wanted_files[filename].num_segments; ++j) {
            details_wanted_files[filename].owned_segments.set(j, false);
        }

    }

    for (auto& [filename, m] : details_wanted_files) {
        // print the swarm
        
        for (int s = 0; s < m.num_segments; ++s) {
            // every tenth segment, ask for an updated swarm
            if (s % 10 == 0) {
                // ask the tracker for the updated swarm for the file
                MPI_Send(filename.c_str(), MAX_FILENAME, MPI_CHAR, TRACKER_RANK, TAG_UPDATE_SWARM, MPI_COMM_WORLD);

                int swarm_size;
                MPI_Recv(&swarm_size, 1, MPI_INT, TRACKER_RANK, TAG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                m.swarm.resize(swarm_size);
                for (int i = 0; i < swarm_size; ++i) {
                    MPI_Recv(&m.swarm[i], 1, MPI_INT, TRACKER_RANK, TAG_UPDATE_SWARM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }

            // ask a random peer for the segment
            int peer = m.swarm[rand() % m.swarm.size()];

            send_request(filename, s, peer, MPI_COMM_WORLD);
            string response = receive_response(peer, MPI_COMM_WORLD);

            while (strcmp(response.c_str(), "NO") == 0) {
                
                // try the previous peer (closer to the beginning of the swarm are the peers that have all the file)
                peer--;
                send_request(filename, s, peer, MPI_COMM_WORLD);
                response = receive_response(peer, MPI_COMM_WORLD);
            }


            if (strcmp(response.c_str(), "OK") == 0) {
                string segment = segment_response(peer, m, s);

                // check if the segment is correct
                if (strcmp(segment.c_str(), m.hashes[s]) == 0) {
                    m.owned_segments.set(s, true);
                }
            }
        }

        // check if the file is complete
        bool complete = true;
        for (int i = 0; i < m.num_segments; ++i) {
            if (!m.owned_segments[i]) {
                complete = false;
                break;
            }
        }

        if (complete) {
            // send to the tracker that the file is complete
            int msg = 1;
        }
    }

    for (auto& [filename, m] : details_wanted_files) {
        ofstream out("client" + to_string(args->rank) + "_" + filename);
        for (int i = 0; i < m.num_segments; ++i) {
            out << m.hashes[i] << endl;
        }
        out.close();
    }

    int msg2 = 1;
    MPI_Send(&msg2, 1, MPI_INT, TRACKER_RANK, TAG_FINISH_DOWNLOAD, MPI_COMM_WORLD);

    return NULL;
}

void *upload_thread_func(void *arg)
{
    upload_args *args = (upload_args*) arg;

    while (1) {
        MPI_Status status;

        int le;
        
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_REQUEST) {

            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, TAG_REQUEST, MPI_COMM_WORLD, &status);

            int segIndex;
            MPI_Recv(&segIndex, 1, MPI_INT, status.MPI_SOURCE, TAG_REQUEST, MPI_COMM_WORLD, &status);

            // search for the file
            metadata m;
            for (int i = 0; i < args->owned_files.size(); ++i) {
                if (strcmp(args->owned_files[i].filename, filename) == 0) {
                    m = args->owned_files[i];
                    break;
                }
            }

            if (m.owned_segments[segIndex]) {
                const char* ackMsg = "OK";
                MPI_Send(ackMsg, 4, MPI_CHAR, status.MPI_SOURCE, TAG_RESPONSE, MPI_COMM_WORLD);

                // send the segment
                MPI_Send(m.hashes[segIndex], HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, TAG_RESPONSE, MPI_COMM_WORLD);
                
            } else {
                const char* ackMsg = "NO";
                MPI_Send(ackMsg, 4, MPI_CHAR, status.MPI_SOURCE, TAG_RESPONSE, MPI_COMM_WORLD);
            }

        } else if (status.MPI_TAG == TAG_UPLOAD_CLOSE) {
            int msg;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TAG_UPLOAD_CLOSE, MPI_COMM_WORLD, &status);
            break;
        }
    }

    return NULL;
}

void tracker(int numtasks, int rank) {
    vector<metadata> all_files;
    int count_incoming_peers = 0;
    int count_downloading_peers = 0;

    while (1) {
        if (count_incoming_peers == numtasks - 1) {
            // send a broadcast to all peers (they can start downloading)
            int msg = 1;
            
            for (int i = 1; i < numtasks; ++i) {
                MPI_Send(&msg, 1, MPI_INT, i, TAG_START_DOWNLOAD, MPI_COMM_WORLD);
            }

            count_incoming_peers = -2000000;
        }


        MPI_Status status;

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (status.MPI_TAG == TAG_OWNED_FILES) {

            metadata m;
            MPI_Recv(&m, sizeof(metadata), MPI_BYTE, MPI_ANY_SOURCE, TAG_OWNED_FILES, MPI_COMM_WORLD, &status);
            m.swarm.push_back(status.MPI_SOURCE);
            all_files.push_back(m);

        } else if (status.MPI_TAG == TAG_DOWNLOAD_FILE) {

            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD, &status);

            // search for the file
            metadata m;
            for (int i = 0; i < all_files.size(); ++i) {
                if (strcmp(all_files[i].filename, filename) == 0) {
                    m = all_files[i];
                    break;
                }
            }

            send_metadata(m, status.MPI_SOURCE, MPI_COMM_WORLD);
            MPI_Send(&m.num_segments, 1, MPI_INT, status.MPI_SOURCE, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD);

            for (int j = 0; j < m.num_segments; ++j) {
                MPI_Send(m.hashes[j], HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, TAG_DOWNLOAD_FILE, MPI_COMM_WORLD);
            }

            // add the peer to the swarm
            m.swarm.push_back(status.MPI_SOURCE);

            printf("Tracker, added peer %d to the swarm of file %s\n", status.MPI_SOURCE, filename);

        } else if (status.MPI_TAG == TAG_FINISH_DOWNLOAD) {
            
            int msg;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TAG_FINISH_DOWNLOAD, MPI_COMM_WORLD, &status);

            count_downloading_peers++;

            if (count_downloading_peers == numtasks - 1) {
                // close upload threads
                int msg = 1;
            
                for (int i = 1; i < numtasks; ++i) {
                    MPI_Send(&msg, 1, MPI_INT, i, TAG_UPLOAD_CLOSE, MPI_COMM_WORLD);
                }
                break;
            }

        } else if (status.MPI_TAG == TAG_INCOMING_PEER) {
            int msg;
            MPI_Recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, TAG_INCOMING_PEER, MPI_COMM_WORLD, &status);
            count_incoming_peers++;

        } else if (status.MPI_TAG == TAG_UPDATE_SWARM) {
            char filename[MAX_FILENAME];
            MPI_Recv(filename, MAX_FILENAME, MPI_CHAR, MPI_ANY_SOURCE, TAG_UPDATE_SWARM, MPI_COMM_WORLD, &status);

            // search for the file
            metadata m;
            for (int i = 0; i < all_files.size(); ++i) {
                if (strcmp(all_files[i].filename, filename) == 0) {
                    m = all_files[i];
                    break;
                }
            }

            // send the updated swarm
            int swarm_size = m.swarm.size();
            MPI_Send(&swarm_size, 1, MPI_INT, status.MPI_SOURCE, TAG_UPDATE_SWARM, MPI_COMM_WORLD);
            for (int i = 0; i < swarm_size; ++i) {
                MPI_Send(&m.swarm[i], 1, MPI_INT, status.MPI_SOURCE, TAG_UPDATE_SWARM, MPI_COMM_WORLD);
            }
        }
    }

}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;
    void *status;
    int r;

    vector<metadata> owned_files;
    vector<string> wanted_files;

    // Read input from file
    // FILE *in = fopen(("../checker/tests/test1/in" + to_string(rank) + ".txt").c_str(), "r");
    FILE *in = fopen(("in" + to_string(rank) + ".txt").c_str(), "r");

    int num_owned_files;
    fscanf(in, "%d", &num_owned_files);

    for (int i = 0; i < num_owned_files; ++i) {
        metadata m;
        fscanf(in, "%s %d", m.filename, &m.num_segments);

        for (int j = 0; j < m.num_segments; ++j) {
            fscanf(in, "%s", m.hashes[j]);
        }

        for (int j = 0; j < m.num_segments; ++j) {
            m.owned_segments.set(j, true);
        }

        owned_files.push_back(m);
    }

    int num_wanted_files;
    fscanf(in, "%d", &num_wanted_files);

    for (int i = 0; i < num_wanted_files; ++i) {
        char filename[MAX_FILENAME];
        fscanf(in, "%s", filename);
        wanted_files.push_back(string(filename));
    }

    fclose(in);

    // send the owned files to the tracker
    for (int i = 0; i < owned_files.size(); ++i) {
        metadata m = owned_files[i];
        MPI_Send(&m, sizeof(metadata), MPI_BYTE, TRACKER_RANK, TAG_OWNED_FILES, MPI_COMM_WORLD);
    }

    // send incoming peers to the tracker
    int msg = 1;
    MPI_Send(&msg, 1, MPI_INT, TRACKER_RANK, TAG_INCOMING_PEER, MPI_COMM_WORLD);

    // receive the go signal from the tracker
    MPI_Recv(&msg, 1, MPI_INT, TRACKER_RANK, TAG_START_DOWNLOAD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


    struct download_args args;
    args.rank = rank;
    args.wanted_files = wanted_files;
    args.end = 0;

    upload_args u_args;
    u_args.rank = rank;
    u_args.owned_files = owned_files;
    u_args.end = args.end;

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) &args);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) &u_args);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }    

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
    
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}