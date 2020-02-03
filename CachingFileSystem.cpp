/*
 * CachingFileSystem.cpp
 */

#define FUSE_USE_VERSION 26


#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
//bbfs includes
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>

//our includes
#include <fcntl.h>
//#include <sys/types.h>
//#include <attr.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <malloc.h>

#include <bits/ios_base.h>
#include <fstream>
#include <iostream>
#include <errno.h>
#include <fuse.h>


//todo remove
//#define PATH_MAX 4096

using namespace std;
//#define DEBUG
//#define VV

#define FS_LOG(instr)  {\
ostringstream stream;\
cout << instr << "\n";\
stream << instr << "\n";\
log(stream.str());}

#ifdef DEBUG
// LOGGING STUFF
//todo: notice: the scope is just for a local var decl'
#define DEBUG_LOG(instr)  {\
ostringstream stream;\
cout << instr << "\n";\
stream << instr << "\n";\
debug_log(stream.str());}
#ifdef VV
#define VV_LOG(str) DEBUG_LOG(str)
#else
#define VV_LOG(str) /**/
#endif
#else
#define DEBUG_LOG(str) /**/
#define VV_LOG(str) /**/
#endif

#ifdef DEBUG
#define DEBUG_PRINT(str) std::cout<< "(not logged)" << str << std::endl;
#else
#define DEBUG_PRINT(str) /**/
#endif

//// GLOBALS ===================================================================

static int blksz;
static char log_pth[PATH_MAX];
static char *rootdir;
static std::ofstream logFile;
static std::ofstream logFile2;
static std::ofstream debug_file;
static int global_file_handle = -1;

void log(string str)
{

    logFile << str;
    logFile.flush();
    logFile2 << str;
    logFile2.flush();

}

void debug_log(string str)
{
    debug_file << str;
    debug_file.flush();

}

void log_init()
{
    logFile.open(log_pth, std::ios_base::app);
    logFile2.open("./zzz_backup_log", std::ios_base::out | std::ios_base::app);
    logFile2 <<
    "\n ###################################################### :)\n";
    debug_file.open("./zzz_debug_log", std::ios_base::out);
//    log_file.open(".filesystem.log", std::ios_base::app | std::ios_base::out);
//    d_log_file.open("./.debug.log", std::ios_base::out);
    DEBUG_LOG("Logging started")
}

//todo notice! the naive lib has no knowledge of root/mount dirs. It handles only absolute paths
void log_finalize()
{
    DEBUG_LOG("log finished")
    DEBUG_LOG("------------")
    logFile.close();
    logFile2.close();
    debug_file.close();
}


//
//START CACHE CODE ############################################################
//

//**Defines**//
#include <iostream>
#include <list>
#include <map>
#include <malloc.h>
#include <algorithm>
#include <unistd.h>
#include <string.h>
#include <sstream>
//#include "cache.h"
#define INIT_REF_COUNT 1

using namespace std;
enum Lst_type {
    NEW, MID, OLD
};
typedef struct node_info {
    int block_num;
    int ref_count = INIT_REF_COUNT;
    Lst_type type = NEW;
//	list<node_info*>* lst_ptr;
    string path;
    ssize_t size_of_block;
    char *data = NULL;

    node_info(int blknm, size_t realsz, string pth, char *dt)
    {
        block_num = blknm;
        ref_count = INIT_REF_COUNT;
        type = NEW;
        path.assign(pth);
        size_of_block = realsz;
        data = dt;
    }
} node_info;

//A comparison operator for the multimap.
struct str_cmp {
    bool operator()(const string s1, const string s2) const
    {
        return (s1) < (s2);
    }
};

struct int_cmp {
    bool operator()(const int s1, const int s2) const
    {
        return (s1) < (s2);
    }
};

typedef map<string, map<int, int, int_cmp>, str_cmp> MAP_PATH_TO_BLOCK_PTR;
typedef list<int> NEW_CACHE_PARTITION_LST;
typedef list<int> MID_CACHE_PARTITION_LST;
typedef list<int> OLD_CACHE_PARTITION_LST;
typedef list<int> CACHE_PARTITION_LST;
//typedef map<string, list<node_info*>*>::iterator MAP_PATH_TO_BLOCK_ITR;
typedef MAP_PATH_TO_BLOCK_PTR::iterator MAP_PATH_TO_BLOCK_ITR;

//**Globals**//
static MAP_PATH_TO_BLOCK_PTR path_lst_ptr_map;
static NEW_CACHE_PARTITION_LST lst_new_cache;
static MID_CACHE_PARTITION_LST lst_mid_cache;
static OLD_CACHE_PARTITION_LST lst_old_cache;

static vector<pair<node_info, int>> physical_memory;

static unsigned int lst_new_cache_size;
static unsigned lst_mid_cache_size;
static unsigned lst_old_cache_size;

static __blksize_t blk_size;

static int numberOfBlocks;
static float fOld;
static float fNew;

int check_addr(int addr)
{
    if (addr > (numberOfBlocks - 1))
    {
        DEBUG_LOG(__FUNCTION__ << " trying to access bad addres")
//		return -1;
    }
//	if(physical_memory[addr].second==-1){
//		DEBUG_LOG(__FUNCTION__<<" trying to access empty addres")
//		return -1;
//	}
    return addr;
}

//**Helper Functions**//
size_t next_multiple(size_t base, size_t block_size)
{
    size_t div = base / block_size;
    if (base % block_size > 0)
    {
        div++;
    }
    return block_size * div;
}



//From map.
int get_block_ptr(string path, int block_index)
{
    int block_ptr;// = NULL;

    for (map<int, int>::const_iterator it = path_lst_ptr_map[path].begin(),
                 end = path_lst_ptr_map[path].end();
         it != end; ++it)
    {

        int current = (*it).second;
        check_addr(current);
        //DEBUG_PRINT("Trying to match with: " << current -> block_num)

        if (physical_memory[current].first.block_num == block_index)
        {
            block_ptr = (*it).second;
        }
    }

    return block_ptr;
}


//std::list::sort is stable (standard, section 23.2.24).
void lst_sort_by_ref_count(CACHE_PARTITION_LST *lst)
{
    (*lst).sort([](const int a, const int b) {
        return physical_memory[check_addr(a)].first.ref_count >
               physical_memory[check_addr(b)].first.ref_count;
    });
}


void update_cache_existing_block(int block_index)
{
    int tmp_block_index, tmp_new_lst_front_index, tmp_mid_lst_front_index;
    //Todo: Free memory of 'new node_info'.

    if (physical_memory[check_addr(block_index)].first.type !=
        NEW) //MID or OLD.
    {
        physical_memory[block_index].first.ref_count++;
    }

    list<int>::iterator block_iter;
    list<int> *lst_ptr;

    if (physical_memory[check_addr(block_index)].first.type == NEW)
    {
        block_iter = std::find(lst_new_cache.begin(), lst_new_cache.end(),
                               block_index);
        lst_ptr = &lst_new_cache;
    }
    else if (physical_memory[check_addr(block_index)].first.type == MID)
    {
        block_iter = std::find(lst_mid_cache.begin(), lst_mid_cache.end(),
                               block_index);
        lst_ptr = &lst_mid_cache;
    }
    else if (physical_memory[check_addr(block_index)].first.type == OLD)
    {
        block_iter = std::find(lst_old_cache.begin(), lst_old_cache.end(),
                               block_index);
        lst_ptr = &lst_old_cache;
    }


    if ((lst_new_cache_size >= (lst_new_cache.size() + 1)) ||
        (lst_new_cache_size == lst_new_cache.size() &&
         physical_memory[block_index].first.type ==
         NEW)) //Free space or Reposition NEW element.
    {
        tmp_block_index = *block_iter; //Get ptr.
        lst_ptr->erase(block_iter);
        lst_new_cache.push_back(tmp_block_index);

    } else if (lst_mid_cache_size >= (lst_mid_cache.size() + 1))
    {
        tmp_block_index = *block_iter; //Get ptr.
        lst_ptr->erase(block_iter);
        tmp_new_lst_front_index = lst_new_cache.front();
        lst_new_cache.pop_front();

        physical_memory[check_addr(tmp_block_index)].first.type = NEW;
        lst_new_cache.push_back(tmp_block_index);

        physical_memory[check_addr(tmp_new_lst_front_index)].first.type = MID;
        lst_mid_cache.push_back(tmp_new_lst_front_index);

    } else
    {
        tmp_block_index = *block_iter; //Get ptr.
        lst_ptr->erase(block_iter);

        tmp_new_lst_front_index = lst_new_cache.front();
        lst_new_cache.pop_front();

        tmp_mid_lst_front_index = lst_mid_cache.front();
        lst_mid_cache.pop_front();

        physical_memory[check_addr(tmp_block_index)].first.type = NEW;
        lst_new_cache.push_back(tmp_block_index);

        physical_memory[check_addr(tmp_new_lst_front_index)].first.type = MID;
        lst_mid_cache.push_back(tmp_new_lst_front_index);

        physical_memory[check_addr(tmp_mid_lst_front_index)].first.type = OLD;
        lst_old_cache.push_back(tmp_mid_lst_front_index);
    }


}

void update_cache_new_block(int block_position_in_file, string path,
                            char *tmp_buf, ssize_t size_of_block)
{
    int block_ptr, tmp_new_lst_front, tmp_mid_lst_front, tmp_old_lst_front;

//	string s_pth(path);
//	node_info new_block = {
//			block_position_in_file,//.block_num =
//			INIT_REF_COUNT,
//			NEW, // type
//			s_pth, // path todo mbe error with mem
//			size_of_block, //.size_of_block =
//			tmp_buf //.data =
//
//
////			int block_num;
////			int ref_count = INIT_REF_COUNT;
////			Lst_type type = NEW;
//////	list<node_info*>* lst_ptr;
////			string path;
////			ssize_t size_of_block;
////			char* data = NULL;
//	};
    node_info new_block(block_position_in_file, size_of_block, path, tmp_buf);
    physical_memory.push_back({new_block, 1});
    block_ptr = physical_memory.size() - 1;

    if (lst_new_cache_size >= (lst_new_cache.size() + 1)) //Can add new block.
    {

        lst_new_cache.push_back(block_ptr);
        //Todo: Free memory of 'new node_info'.
        //Need to take out a block or/and rearrange cache.
    } else if (lst_mid_cache_size >= (lst_mid_cache.size() + 1))
    {
        //pop the oldest block 'A' from the new list
        tmp_new_lst_front = lst_new_cache.front();
        lst_new_cache.pop_front();

        // push the current new block to the new list
        lst_new_cache.push_back(block_ptr);

        //push 'A' as the newest block to the mid list
        physical_memory[tmp_new_lst_front].first.type = MID;
        lst_mid_cache.push_back(tmp_new_lst_front);
        //Todo: Free memory of 'new node_info'.
    } else if (lst_old_cache_size >= (lst_old_cache.size() + 1))
    {
        // peak A
        tmp_new_lst_front = lst_new_cache.front();

        //pop the oldest block 'A' from the new list
        lst_new_cache.pop_front();

        //pop B
        tmp_mid_lst_front = lst_mid_cache.front();
        lst_mid_cache.pop_front();

        // push 'NEW BLOCK' to new
        lst_new_cache.push_back(block_ptr);

        // change A type to MID
        physical_memory[tmp_new_lst_front].first.type = MID;
        //push A to MID
        lst_mid_cache.push_back(tmp_new_lst_front);
        // change type B to OLD
        physical_memory[tmp_mid_lst_front].first.type = OLD;
        // push B to OLD
        lst_old_cache.push_back(tmp_mid_lst_front);

        //Todo: Free memory of 'new node_info'.
    } else
    { //All lists are full.
        // pop A
        tmp_new_lst_front = lst_new_cache.front();
        lst_new_cache.pop_front();
        // pop B
        tmp_mid_lst_front = lst_mid_cache.front();
        lst_mid_cache.pop_front();
        // pass gloabl list addr to sorter
        lst_sort_by_ref_count(&lst_old_cache); //Smallest ref-count last.
        //pop C
        tmp_old_lst_front = lst_old_cache.front();
        lst_old_cache.pop_front();
        // push NEW to NEW
        lst_new_cache.push_back(block_ptr);

        // A -> MID
        physical_memory[tmp_new_lst_front].first.type = MID;
        lst_mid_cache.push_back(tmp_new_lst_front);

        //B -> OLD
        physical_memory[tmp_mid_lst_front].first.type = OLD;
        lst_old_cache.push_back(tmp_mid_lst_front);
        // remove C from global MAP
        int C_BLNUM = physical_memory[tmp_old_lst_front].first.block_num;
        (path_lst_ptr_map[path]).erase(C_BLNUM);
        // free
        delete[] (physical_memory[tmp_old_lst_front].first.data); // TODO

    }
    path_lst_ptr_map[path].insert({block_position_in_file, block_ptr});
//    cout << cache_to_string() << endl;
}

//All of our block logic works with aligned buffer.
ssize_t retrive_all_info(int fd, string path, char *aligned_buf,
                         size_t low_bound,
                         size_t up_bound, size_t offset_requested)
{
    ssize_t retstat = 0, tmp_retstat = 0;
    //TODO : this is not required.. you get a rounded input always!
//	list<int> *lst_found_blocks_nums;
    size_t total_mem_size =
            up_bound - low_bound;// heara suspect!! low bound!!! todo
    size_t end_block;
    size_t start_block = (low_bound / (size_t) blk_size);
    //size_t addition_for_sub = 0;
    int block_ptr;
    char *tmp_buf;

    end_block = (total_mem_size / (size_t) blk_size) + start_block;

    for (int i = (int) start_block; i < (int) end_block; i++)
    {
        bool found = false;
        //One block at a time.
//		lst_found_blocks_nums = return_found_cash_blocks(path);
        if (path_lst_ptr_map[path].find(i) != path_lst_ptr_map[path].end())
        {
            found = true;
        }
        if (found)
        {
            block_ptr = get_block_ptr(path, i); //it val is the ptr.

//            int where = (i - (int)start_block) * blk_size;
//            char  * c_data= block_ptr -> data;
//            memcpy(&aligned_buf[where],c_data, (size_t) blk_size);
//			int correct_to_read = (block_ptr -> size_of_block) -
// (offset_requested % blk_size);
            size_t filled_blocks_so_far =
                    physical_memory[check_addr(block_ptr)].first.block_num *
                    blk_size;
//			if(filled_blocks_so_far>offset_requested)
            //todo error prone rem = -1 <- when filled>ofs
            long rem = offset_requested - filled_blocks_so_far;
            if (filled_blocks_so_far > offset_requested)
            {
                rem = -1;
            }

            if (rem <
                physical_memory[check_addr(block_ptr)].first.size_of_block)
            {
                memcpy(&aligned_buf[(i - (int) start_block) * blk_size],
                       physical_memory[check_addr(block_ptr)].first.data,
                       (size_t) blk_size);
//			memcpy(&aligned_buf[(i - (int)start_block) * blk_size],
// block_ptr -> data, correct_to_read);

//			retstat += block_ptr -> size_of_block - (offset_requested%blk_size);
//				retstat += blk_size;
                retstat += physical_memory[check_addr(
                        block_ptr)].first.size_of_block;
                /*int = */update_cache_existing_block(block_ptr);
            } else
            { break; }
        } else
        { //Need to bring block from fs.
//			int curofs =i * blk_size;
            tmp_retstat = pread(fd, &aligned_buf[(i - (int) start_block) *
                                                 blk_size], (size_t) blk_size,
                                i *
                                blk_size); //Update aligned buffer with the
            // new data from the fs.

            if (tmp_retstat > 0)
            {
                retstat += tmp_retstat;
            } else if (tmp_retstat == 0)
            {
                break;
            }
            else
            {
                return tmp_retstat;
            }

            tmp_buf = new char[(size_t) blk_size];
            memcpy(tmp_buf, &aligned_buf[(i - (int) start_block) * blk_size],
                   tmp_retstat);
            /*int = */update_cache_new_block(i, path, tmp_buf, tmp_retstat);
            //Todo: delete[] tmp_buf;
        }
//		delete(lst_found_blocks_nums);
    }

    return retstat;
}

void destroy_cache()
{
    //Todo: Free all memory from lists!.
    //Todo: Free block_ptr -> data from all nodes inside lists! (tmp_buf).

    for (MAP_PATH_TO_BLOCK_ITR map_it = path_lst_ptr_map.begin();
         map_it != path_lst_ptr_map.end(); map_it++)
    {
//		for (auto lst_it = (*map_it).second->begin(),
//					 end = (*map_it).second->end(); lst_it != end; ++lst_it)
//		{
//			//DEBUG_PRINT((*lst_it)->data)
//			//DEBUG_PRINT((*lst_it)->lst_ptr)
//			delete[]((*lst_it)->data);
//			//delete((*lst_it)->lst_ptr);
//			delete(*lst_it);
//		}
        //DEBUG_PRINT((*map_it).second)
//		delete((*map_it).second);
    }
    path_lst_ptr_map.clear();
}

//**API Functions**//
/**
 * Description:
 * Params:
 * Return Value: a Non-negative integer indicating the number of bytes actually
 * read. Otherwise, the functions shall return -1 and set errno to indicate the
 * error.
 */
ssize_t read_cache(int fd, char *path, char *result_buf,
                   size_t requested_size_to_read, size_t offset_requested)
//ssize_t read_from_cash(string path, int fd, char *result_buf,
// size_t size_to_read, off_t starting_offset, int numberOfBlocks, float fOld,
// float fNew)
{
    //Todo: These values should be dealt in the main.
//	global_file_handle = fd;
    lst_new_cache_size = (unsigned int) (fNew * numberOfBlocks);
    lst_old_cache_size = (unsigned int) (fOld * numberOfBlocks);
    lst_mid_cache_size =
            numberOfBlocks - lst_new_cache_size - lst_old_cache_size;

    string str_path(path);

    ssize_t retstat = 0;  //ssize_t can also receive negative values for errors.
    size_t up_bound = next_multiple(requested_size_to_read + offset_requested,
                                    (size_t) blk_size);
    size_t offset_remainder = (offset_requested % (size_t) blk_size);
    size_t low_bound = offset_requested - offset_remainder;
    size_t buff_size = up_bound - low_bound;

    //Todo: Again dont do this in the cache!!. the input is allways alligned!

    //Align the new memory container for the file.
    char *aligned_buf = (char *) memalign((size_t) blk_size, buff_size);

    //Searches for already saved blocks in memory, else read from fs.
    MAP_PATH_TO_BLOCK_PTR::iterator it = path_lst_ptr_map.find(str_path);
    if (it != path_lst_ptr_map.end()) //If the file's path is in cash.
    {
//        DEBUG_PRINT("Path been here.")
        retstat = retrive_all_info(fd, str_path, aligned_buf, low_bound,
                                   up_bound,
                                   offset_requested);

    } else
    { //If it's a new fd.
//        DEBUG_PRINT("First tme path.")
//		list<int> * lstptr = new list<int>();
//		path_lst_ptr_map[str_path] = lstptr;
        retstat = retrive_all_info(fd, str_path, aligned_buf, low_bound,
                                   up_bound,
                                   offset_requested);
    }

    //After getting all information to 'aligned_buf', copies only needed bytes
    //to 'result_buf'.
    if (retstat >= 0)
    {
        //        dest    |          src
        memcpy(result_buf, &aligned_buf[offset_remainder],
               requested_size_to_read);
        //retstat = strlen(result_buf);
    }
    if (retstat < 0) //In case of an error.
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        retstat = -errno;
    }
    free(aligned_buf);
    if (retstat < (ssize_t) requested_size_to_read)
    {
        retstat -= offset_remainder;
        retstat = max(0, (int) retstat);
    }
    return retstat;
}

void init_cache(int numberOfBlocks, __blksize_t blk_size, float fOld,
                float fNew)
{
    destroy_cache(); //For consecutive runs.
    ::numberOfBlocks = numberOfBlocks;
    ::blk_size = blk_size;
    ::fOld = fOld;
    ::fNew = fNew;
}


/**
 * return value: npos (-1) means no matches were found,
 * else a a value >=0 will be returned.
 */
size_t change_cache_path(char *old_path, char *new_path)
{
    size_t matched_pos = string::npos;
    MAP_PATH_TO_BLOCK_PTR tmp_map;//Updating original map will result iterating
    // over modified path again at the same function run.

    for (MAP_PATH_TO_BLOCK_ITR map_it = path_lst_ptr_map.begin();
         map_it != path_lst_ptr_map.end(); map_it++)
    {
        string str_old(old_path), str_new(new_path), str_tmp;
        if ((matched_pos = (map_it->first.find(str_old))) != string::npos)
        {
            str_tmp.assign(map_it->first);
            str_tmp.replace(matched_pos, str_old.length(), str_new);

            tmp_map[str_tmp] = path_lst_ptr_map[map_it->first];

            for (map<int, int>::iterator blocks_map_it =
                    tmp_map[str_tmp].begin();
                 blocks_map_it != tmp_map[str_tmp].end(); blocks_map_it++)
            {
                physical_memory[(*blocks_map_it).second].first.path.assign(
                        str_tmp);
            }

            path_lst_ptr_map.erase(map_it);

        } else
        {
            tmp_map[map_it->first] = path_lst_ptr_map[map_it->first];
            path_lst_ptr_map.erase(map_it);
        }
    }
    path_lst_ptr_map.insert(tmp_map.begin(), tmp_map.end());
    tmp_map.clear();
    return matched_pos;
}

//string full_to_rel_path(string full_path)
//{
//
//}
//
//string cache_to_string()
//{
//	std::ostringstream stream;
//	string str_rel_path;
//	list<int> *lst_found_blocks_nums;
//
//	for(MAP_PATH_TO_BLOCK_ITR map_it = path_lst_ptr_map.begin();
// map_it != path_lst_ptr_map.end(); map_it++)
//	{
//		str_rel_path = full_to_rel_path(map_it->first);
//		//lst_found_blocks_nums = return_found_cash_blocks(map_it->first);
//
//
//	}
//}

string full_to_rel_path(string full_path)
{
    DEBUG_LOG("<< full_to_rel_path" << full_path)
    string temp_root;
    temp_root.assign(rootdir);
    int len = temp_root.length();

    full_path.erase(0, len + 1);
    DEBUG_LOG("          ret:" << full_path << "                 >>")
    return full_path;
}

string cache_to_string()
{
    std::ostringstream stream;

    for (NEW_CACHE_PARTITION_LST::const_iterator it = lst_old_cache.begin(),
                 end = lst_old_cache.end();
         it != end; ++it)
    {
        stream << full_to_rel_path(physical_memory[(*it)].first.path) << " ";
        stream << (physical_memory[(*it)].first.block_num + 1) << " " <<
        physical_memory[(*it)].first.ref_count << endl;
    }
    for (NEW_CACHE_PARTITION_LST::const_iterator it = lst_mid_cache.begin(),
                 end = lst_mid_cache.end();
         it != end; ++it)
    {
        stream << full_to_rel_path(physical_memory[(*it)].first.path) << " ";
        stream << (physical_memory[(*it)].first.block_num + 1) << " " <<
        physical_memory[(*it)].first.ref_count << endl;
    }
    for (NEW_CACHE_PARTITION_LST::const_iterator it = lst_new_cache.begin(),
                 end = lst_new_cache.end();
         it != end; ++it)
    {
        stream << full_to_rel_path(physical_memory[(*it)].first.path) << " ";
        stream << (physical_memory[(*it)].first.block_num + 1) << " " <<
        physical_memory[(*it)].first.ref_count << endl;
    }
    string str_res = stream.str();
    str_res.erase(str_res.length() - 1, 1);
    return str_res;
}

// END CACHE CODE ##############################################################

#define PATH_MAX 4096

//typedef struct caching_data{
//	int blksz;
//	char * log_pth;
//	char * rootdir;
//	std::ofstream logFile;
//	std::ofstream logFile2;
//	std::ofstream debug_file;
//	int global_file_handle;
//}caching_data;
//
//#define FDATA ((struct caching_data *) fuse_get_context()->private_data)





// static ostringstream stream;
//globals


// static ostringstream stream;
struct fuse_operations caching_oper;

// static ostringstream stream;

int get_fd()
{
    return global_file_handle;
}

void set_fd(int fd)
{
    global_file_handle = fd;
}


bool is_log(const char *path)
{
    if (strcmp(path, log_pth))
    { return false; }
    return true;
}


//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
    DEBUG_LOG("(( fullpath, got buffer : " << fpath << " and path " << path <<
              " current root: " << rootdir)
    strcpy(fpath, rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will break here

//	std::cout<<"bb_fullpath:  rootdir = "<<rootdir<<", path = "<<path<<",
// fpath = "<<fpath<<"\n";

}


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *rpath, struct stat *statbuf)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    //^_^
//	int ret_intern = internal_TESTED_getattr(fpath,statbuf);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " getattr")
    DEBUG_LOG("PATH: " << path)
    DEBUG_LOG("[[---" << __FUNCTION__)
    if (is_log(path))
    { return -ENOENT; }
    int retstat;

    retstat = lstat(path,
                    statbuf);

    DEBUG_LOG("return status: " << retstat)
    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;

    }
    if (retstat == 0)
    {
        VV_LOG(
                "Logging statbuf: +++++++++++++++++++++++++++++++++++++++++++")
        VV_LOG("st_dev: " << statbuf->st_dev << "\n" <<
               "st_ino: " << statbuf->st_ino << "\n" <<
               "st_mode: " << statbuf->st_mode << "\n" <<
               "st_nlink: " << statbuf->st_nlink << "\n" <<
               "st_uid: " << statbuf->st_uid << "\n" <<
               "st_gid: " << statbuf->st_gid << "\n" <<
               "st_rdev: " << statbuf->st_rdev << "\n" <<
               "st_size: " << statbuf->st_size << "\n" <<
               "st_blksize: " << statbuf->st_blksize << "\n" <<
               "st_blocks: " << statbuf->st_blocks << "\n" <<
               "st_atime: " << statbuf->st_atime << "\n" <<
               "st_mtime: " << statbuf->st_mtime << "\n" <<
               "st_ctime: " << statbuf->st_ctime)
        VV_LOG(
                "Done Logging statbuf: +++++++++++++++++++++++++++++++++++++++")
    }

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")


    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *rpath, struct stat *statbuf,
                     struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse. "
                    "Now calling naive function.")
    //^_^
//    int ret_intern = internal_TESTED_fgetattr(fpath,statbuf);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " fgetattr")

    DEBUG_LOG("[[---" << __FUNCTION__)
    int retstat = 0;
    //TODO (ALEX) : need to make sure global file handle isnt some garbage
    // and return error otherwise!
    // its redundant! if gFH is garbage, fstat will complain accordingly
    retstat = fstat(global_file_handle, statbuf);

    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;

    }
    if (retstat == 0)
    {
        VV_LOG(
                "Logging statbuf: +++++++++++++++++++++++++++++++++++++++++++")
        VV_LOG("st_dev: " << statbuf->st_dev << "\n" <<
               "st_ino: " << statbuf->st_ino << "\n" <<
               "st_mode: " << statbuf->st_mode << "\n" <<
               "st_nlink: " << statbuf->st_nlink << "\n" <<
               "st_uid: " << statbuf->st_uid << "\n" <<
               "st_gid: " << statbuf->st_gid << "\n" <<
               "st_rdev: " << statbuf->st_rdev << "\n" <<
               "st_size: " << statbuf->st_size << "\n" <<
               "st_blksize: " << statbuf->st_blksize << "\n" <<
               "st_blocks: " << statbuf->st_blocks << "\n" <<
               "st_atime: " << statbuf->st_atime << "\n" <<
               "st_mtime: " << statbuf->st_mtime << "\n" <<
               "st_ctime: " << statbuf->st_ctime)
        VV_LOG(
                "Done Logging statbuf: +++++++++++++++++++++++++++++++++++++++")
    }

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *rpath, int mask)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    //^_^
//    int ret_intern = internal_TESTED_access(fpath,mask);
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " access")

    DEBUG_LOG("[[---" << __FUNCTION__)
    if (is_log(path))
    { return -ENOENT; }
    int retstat = 0;
    retstat = access(path, mask);

    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;

    }

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * initialize an arbitrary filehandle (fh) in the fuse_file_info 
 * structure, which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *rpath, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)

    if (((fi->flags & O_RDWR) > 0) || ((fi->flags & O_WRONLY) > 0))
    {
        DEBUG_LOG("no access!")
        return -EACCES;
    }
    //^_^
//	int ret_intern = internal_TESTED_open(fpath);
    //--
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " open")

    DEBUG_LOG("[[---" << __FUNCTION__)

    if (is_log(path))
    { return -ENOENT; }
    int retstat = 0;
    int fd;

    fd = open(path, O_DIRECT | O_RDONLY | O_SYNC);
    if (fd < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << fd << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;
        // TODO: what to do with global_FH if error ?! leave it in its
        // previous state?!
//        global_file_handle=-1; //like in bbfs?!
    }

    global_file_handle = fd;

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //--
    if (retstat < 0)
    {
        return retstat;
    }
    DEBUG_LOG("fuse's previous fh:" << fi->fh)
    fi->fh = get_fd();
    fi->flags = O_RDONLY | O_DIRECT | O_SYNC;
    fi->direct_io = 1;
    DEBUG_LOG("Did internal logic, now fuse's fh is:" << fi->fh)
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

//size_t next_multiple(size_t base, size_t block_size)
//{
//	size_t div = base/block_size;
//	if(base % block_size >0){
//		div++;
//	}
//	return block_size*div;
//}


/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error. For example, if you receive size=100, offest=0,
 * but the size of the file is 10, you will init only the first 
   ten bytes in the buff and return the number 10.
   
   In order to read a file from the disk, 
   we strongly advise you to use "pread" rather than "read".
   Pay attention, in pread the offset is valid as long it is 
   a multipication of the block size.
   More specifically, pread returns 0 for negative offset 
   and an offset after the end of the file
   (as long as the the rest of the requirements are fulfiiled).
   You are suppose to preserve this behavior also in your implementation.

 * Changed in version 2.2
 */
int caching_read(const char *rpath, char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse. "
                    "Now calling naive function.")
    //^_^
//	int ret_intern = internal_TESTED_read(fpath,buf,size,offset);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " read")

    DEBUG_LOG("[[---" << __FUNCTION__)

    if (is_log(path))
    { return -ENOENT; }
    ssize_t retstat = 0;

    DEBUG_LOG(
            "@@@ blk " << blksz << " path " << path << " got size: " << size <<
            " off: " << offset)
//	size_t next_mult_of_nbytes=next_multiple(size,blksz);
//
//	size_t offset_remainder = offset%(size_t)blksz;
//	size_t offset_rounded=offset-offset_remainder;
//	size_t buff_size=next_mult_of_nbytes;
//	if(offset_remainder>0){
//		// if we have offset remainder, increase the buffer by 1 block!
//		buff_size=next_mult_of_nbytes+blksz;
//	}
//	DEBUG_LOG("@@@ next mult is: "<<next_mult_of_nbytes<<" buffsz: "<<
//					  buff_size<<" offfrem: "<<offset_remainder<<
//					  " offr: "<<offset_rounded)
//
//	char *alligned_buffer=(char*)memalign(blksz, buff_size);

//	retstat = pread(global_file_handle, alligned_buffer, next_mult_of_nbytes,
// offset_rounded);
//	DEBUG_LOG("@@@ did syscall-pread with return stat: "<<retstat)
    retstat = read_cache(global_file_handle, path, buf, size, offset);
    DEBUG_LOG("@@@ did cache-read with ret: " << retstat)
//	if(retstat>=0){
//		//     dest                  src
//		memcpy(buf, &alligned_buffer[offset_remainder],size);
//	}
    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;
        //---
    } else
    {
        retstat = min((int) retstat, (int) size);
    }
//	free(alligned_buffer);

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *rpath, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse. "
                    "Now calling naive function.")
    //^_^
//	int ret_intern = internal_TESTED_flush(fpath);
    //-
    int retstat = 0;
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " flush")

    DEBUG_LOG("[[---" << __FUNCTION__)
    if (is_log(path))
    { retstat = -ENOENT; }
    //I believe it is related to our cashing, or should be no-op like bbfs.
    //Means - writing the data to the file from the cash before closing fd.

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *rpath, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse. "
                    "Now calling naive function.")
    //^_^
//	int ret_intern = internal_TESTED_release(fpath);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " release")

    DEBUG_LOG("[[---" << __FUNCTION__)
    int retstat = 0;
    retstat = close(global_file_handle);
    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;
    }
    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *rpath, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    //^_^
//	int ret_intern = internal_TESTED_opendir(fpath);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " opendir")

    DEBUG_LOG("[[---" << __FUNCTION__)

    //Variables//
    DIR *dp = opendir(path);
    int retstat = 0;

    //Code//
    if (dp == NULL)
    {
        retstat = -errno;
//        global_file_handle = -1;
    }
    //todo should this be outside else?!
    // perhaps so, its redundant
    global_file_handle = (intptr_t) dp;

    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("fuse's previous fh:" << fi->fh)
    fi->fh = get_fd();
    DEBUG_LOG("Did internal logic, now fuse's fh is:" << fi->fh)
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

bool entry_not_log(const char *abs_path, const char *entry_name)
{
    //strcat
    char root_mod[PATH_MAX];
    strcpy(root_mod, rootdir);
    strcat(root_mod, "/");
    if ((strcmp(abs_path, root_mod) == 0) &&
        (strcmp(entry_name, ".filesystem.log") == 0))
    {
        return false;
    }
    return true;
}


/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *rpath, void *buf,
                    fuse_fill_dir_t filler,
                    off_t offset, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse."
                    " Now calling naive function.")
    //^_^
//	int ret_intern = internal_TESTED_readdir(fpath,buf,filler,offset);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " readdir")

    DEBUG_LOG("[[---" << __FUNCTION__)
    int retstat = 0;
    DIR *dp;

    struct dirent *de;
//	struct fuse_dh *dh;
    DEBUG_LOG("readdir(path=" << path << " offset=" << offset)
    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) global_file_handle;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    if (dp == NULL)
    {
        retstat = -ENOENT;
        return retstat;
    }
    de = readdir(dp);
    DEBUG_LOG("readdir returned " << de);
    if (de == 0)
    {
        retstat = -errno;//log_error("bb_readdir readdir");
        return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do
    {
        DEBUG_LOG("calling {" << filler << "} filler with name " << de->d_name);
        if (filler != NULL)
        {
            if (entry_not_log(path, de->d_name))
            {
                if (filler(buf, de->d_name, NULL, 0) != 0)
                {
                    DEBUG_LOG("ERROR bb_readdir filler:  buffer full");
                    return -ENOMEM;
                }
            }
        }
    } while ((de = readdir(dp)) != NULL);


    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")


    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *rpath, struct fuse_file_info *fi)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    DEBUG_LOG("and fpath: " << path)
    DEBUG_LOG("global fd is : " << get_fd() << ", got fh from fuse: " << fi->fh)
    set_fd(fi->fh);
    DEBUG_LOG(
            "Switched global fd to input from fuse. Now calling naive function.")
    //^_^
//	int ret_intern = internal_TESTED_releasedir(fpath);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " releasedir")

    DEBUG_LOG("[[---" << __FUNCTION__)
    int retstat = 0;
    retstat = closedir((DIR *) (uintptr_t) global_file_handle);
    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;

    }
    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")


    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/** Rename a file */
int caching_rename(const char *rpath, const char *rnewpath)
{
    //T_T
    DEBUG_LOG("<<Starting " << __FUNCTION__ << " with path: " << rpath)
    char path[PATH_MAX];
    bb_fullpath(path, rpath);
    char newpath[PATH_MAX];
    bb_fullpath(newpath, rnewpath);
    DEBUG_LOG("and fpath: " << path)
    //^_^
//	int ret_intern = internal_TESTED_rename(fpath,fnewpath);
    //-
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " rename")

    DEBUG_LOG("[[---" << __FUNCTION__)
    DEBUG_LOG("rename: " << path << " to: " << newpath)
    if (is_log(path) || is_log(newpath))
    { return -ENOENT; }
    int retstat = 0;
    retstat = rename(path, newpath);
    if (retstat < 0)
    {
        /* ltstat():
         * RETURN VALUES Upon successful completion a value of 0 is returned.
         * Otherwise, a value of -1 is returned and errno is set to indicate
         * the error.
         */
        DEBUG_LOG("ERROR: " << retstat << " errno: " << errno << " explain: " <<
                  strerror(errno))
        retstat = -errno;

    }
    change_cache_path(path, newpath);
    //FUNC_END MARKER
    DEBUG_LOG("                     " << __FUNCTION__ << "--]]")

    //-
    DEBUG_LOG("					-- Ending with ret: " << retstat <<
              " >>\n_______________c")
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 
If a failure occurs in this function, do nothing (absorb the failure 
and don't report it). 
For your task, the function needs to return NULL always 
(if you do something else, be sure to use the fuse_context correctly).
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn)
{
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " init")

    DEBUG_LOG("[[---" << __FUNCTION__)
    return NULL;
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
  
If a failure occurs in this function, do nothing 
(absorb the failure and don't report it). 
 
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " destroy")

    DEBUG_LOG("[[---" << __FUNCTION__)
}


/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print 
 cache table to the log file .
 * 
 * Introduced in version 2.8
 */
int caching_ioctl(const char *, int cmd, void *arg,
                  struct fuse_file_info *, unsigned int flags, void *data)
{
    //FUNC_START_MARKER (for sublime text ^_^ )
    time_t unixTime = time(NULL);
    FS_LOG(unixTime << " ioctl")
    FS_LOG(cache_to_string())

//"[[---"<<__FUNCTION__)
    DEBUG_LOG(" Just a printing func. Print cache table... ")
    return 0;
}


// Initialise the operations. 
// You are not supposed to change this function.
void init_caching_oper()
{

    caching_oper.getattr = caching_getattr;
    caching_oper.access = caching_access;
    caching_oper.open = caching_open;
    caching_oper.read = caching_read;
    caching_oper.flush = caching_flush;
    caching_oper.release = caching_release;
    caching_oper.opendir = caching_opendir;
    caching_oper.readdir = caching_readdir;
    caching_oper.releasedir = caching_releasedir;
    caching_oper.rename = caching_rename;
    caching_oper.init = caching_init;
    caching_oper.destroy = caching_destroy;
    caching_oper.ioctl = caching_ioctl;
    caching_oper.fgetattr = caching_fgetattr;


    caching_oper.readlink = NULL;
    caching_oper.getdir = NULL;
    caching_oper.mknod = NULL;
    caching_oper.mkdir = NULL;
    caching_oper.unlink = NULL;
    caching_oper.rmdir = NULL;
    caching_oper.symlink = NULL;
    caching_oper.link = NULL;
    caching_oper.chmod = NULL;
    caching_oper.chown = NULL;
    caching_oper.truncate = NULL;
    caching_oper.utime = NULL;
    caching_oper.write = NULL;
    caching_oper.statfs = NULL;
    caching_oper.fsync = NULL;
    caching_oper.setxattr = NULL;
    caching_oper.getxattr = NULL;
    caching_oper.listxattr = NULL;
    caching_oper.removexattr = NULL;
    caching_oper.fsyncdir = NULL;
    caching_oper.create = NULL;
    caching_oper.ftruncate = NULL;
}

bool is_legal_dirname(const char *path)
{
    struct stat buf;
    bool result = true;
    int error = stat(path, &buf);

    if (error == (-1) || !S_ISDIR(buf.st_mode))
    {
        result = false;
    }
    return result;
}

bool check_valid(int argc, char *argv[])
{
//	DEBUG_PRINT("check valid{")
    if (argc < 6)
    {
//		DEBUG_PRINT("bad num args too few" << argc)
        return false;
    }
    if (argc > 6)
    {
//		DEBUG_PRINT("bad num args too many: " << argc)
        return false;
    }
//	DEBUG_PRINT("dir")
    if (!is_legal_dirname(argv[1]))
    {
//		DEBUG_PRINT("bad rootdir")
        return false;
    }
//	DEBUG_PRINT("dir")
    if (!is_legal_dirname(argv[2]))
    {
//		DEBUG_PRINT("bad mountdir")
        return false;
    }
//	DEBUG_PRINT("atoi")
    // Checking that the number of blocks is a valid number.
    int num_b = atoi(argv[3]);
    if (num_b <= 0)
    {
//		DEBUG_PRINT("bad num blocks")
        return false;
    }
//	DEBUG_PRINT("sum:")
    double fOld = atof(argv[4]), fNew = atof(argv[5]);
    if (fNew + fOld > 1)
    {
//		DEBUG_PRINT("bad sum of new old")
        return false;
    }
//	DEBUG_PRINT("sz")
    int size_old = fOld * num_b, size_new = fNew * num_b;
    if (size_new <= 0 || size_new >= num_b)
    {
//		DEBUG_PRINT("bad size new")
        return false;
    }
    if (size_old <= 0 || size_old >= num_b)
    {
//		DEBUG_PRINT("bad old")
        return false;
    }
//	DEBUG_PRINT("}check valid end")
    return true;
}

//basic main. You need to complete it.
int main(int argc, char *argv[])
{

    if (!check_valid(argc, argv))
    {
        cout <<
        "Usage: CachingFileSystem rootdir mountdir numberOfBlocks fOld fNew" <<
        endl;
        exit(1);
    }


    rootdir = realpath(argv[1], NULL);
    DEBUG_PRINT("realroot: " << rootdir << " , $1: " << argv[1] << " , $2:" <<
                argv[2])

    sprintf(log_pth, "%s%s", rootdir, "/.filesystem.log");
    DEBUG_PRINT(log_pth)
    log_init();


    //Find machine block size and store it globally.
    struct stat fi;
    if (stat("/tmp", &fi) < 0)
    {
        cerr << "System Error: block size" << endl;
        exit(1);
    }
    blksz = fi.st_blksize;
    //          num bloc    ,     ,  fOld       ,   fNew
    init_cache(atoi(argv[3]), blksz, atof(argv[4]), atof(argv[5]));
    //---
    init_caching_oper();

    argv[1] = argv[2];
    for (int i = 2; i < (argc - 1); i++)
    {
        argv[i] = NULL;
    }

    argv[2] = (char *) "-s";
    argc = 3;

//     argv[3]= (char*) "-f";
//	 argc = 4;

    int fuse_stat = fuse_main(argc, argv, &caching_oper, NULL);
    log_finalize();
    return fuse_stat;
}
