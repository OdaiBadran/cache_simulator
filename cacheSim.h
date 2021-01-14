//
// Created by student on 12/12/20.
//

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <bitset>


#ifndef HW2ARCHFINAL_CACHESIM_H
#define HW2ARCHFINAL_CACHESIM_H

using namespace std;
enum CacheResult {HIT, MISS};
class L2;

/**
 * Cache Class.
 * represents a cache which is
 * the parent class of caches L1 and L2.
 */
class Cache {
public:
    int blockSize_;
    int cacheSize_;
    int writeAlloc_;
    int assoc_;
    int cyc_;
    int hit_;
    int miss_;
    bool* dirty_;
    bool* valid_;
    string* cache_;
    int* tag_;
    int* lru_;
    const int MAX_BITS  = 32;

/**
 * Cache constructor.
 * @param blockSize
 * @param cacheSize
 * @param writeAlloc
 * @param assoc
 * @param cyc
 * @param hit
 * @param miss
 */
    Cache(int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
          int hit , int miss);

/**
 * Cache destructor.
 */
    ~Cache();

/**
 * initializeCache method.
 * @job initialize the Cache.
 * @effects initialize dirty,valid,lru all to
 * falses and zeros accordingly.
 */
    void initializeCache();

/**
 * getSet method.
 * @job from the given address get the set.
 * @param address
 * @return the set in decimal.
 */
    int getSet(string& address);

/**
 * getTag method.
 * @job from the given address get the tag
 * @param address
 * @return the tag in decimal.
 */
    int getTag(string& address);

/**
 * update LRU method.
 * @job to update the LRU of each
 * accessed block in the cache.
 * @param set, the set which gotten from the given address
 * @param prior, the specific index of the block which
 * its LRU is updated. usually equal set + (i * waySize)
 * such that waySize equal (cachSize/blockSize)/assoc.
 */
    void updateLru(int set, int prior);

/**
 * Search method.
 * @job search of the address in the Cache.
 * @param address
 * @return HIT if address exist otherwise MISS.
 */
    CacheResult Search(string& address);

};



/**
 * L1 cache.
 * represent L1 cache in the hierarchy.
 * heirs form Cache Class.
 * @author Odai Badran
 * @since 9/12/2020
 */
class L1 : public Cache {
public:

/**
 * L1 Constructor.
 * @param blockSize
 * @param cacheSize
 * @param writeAlloc
 * @param assoc
 * @param cyc
 * @param hit
 * @param miss
 */
    L1(int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
       int hit, int miss);
    ~L1(){}
/**
 * snoop method.
 * @job check wither the block which
 * suits the given address is dirty of not,
 * if dirty write block back to L2 and invalidate
 * the block, otherwise invalidate the block if exist.
 * @param address
 * @param l2
 */
    void snoop(string& address, L2& l2);

/**
 * write to L1 method.
 * @job to write the block suitable for the
 * given address in L1.(also checks if dirty
 * and behaves accordingly)
 * @param address
 * @param l2
 * @param isHitInL1, if there is HIT in L1 the true,
 * false otherwise.
 * @param mode, wither it is read activity it is 'r',
 * otherwise 'w'.
 */
    void writeToL1(string& address, L2& l2, bool isHitInL1, char mode);

};


/**
 * L2 cache.
 * represent L2 cache in the hierarchy.
 * heirs form Cache Class.
 * @author Odai Badran
 * @since 9/12/2020
 */
class L2 : public Cache {
public:

/**
 * L2 Constructor.
 * @param blockSize
 * @param cacheSize
 * @param writeAlloc
 * @param assoc
 * @param cyc
 * @param hit
 * @param miss
 */
    L2(int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
       int hit, int miss);
    ~L2(){}

/**
 * write to L2 and snoop L1.
 * @job to write block of the given address to L2,
 * but snoop L1 first.
 * @param address
 * @param l1
 */
    void writeToL2SnoopL1(string& address, L1 &l1);

/**
 * write dirty blcok to L2 method.
 * @job known from its name.
 * @param address
 */
    void writeDirtyBlockToL2(string& address);


};

/**
 * Memory
 * @author Odai Badran
 * @since 9/12/2020
 */
class Mem{
public:
    int cyc_;
    int access_;

    Mem(int cyc, int access );
    ~Mem() = default;
    /**
     * Bring from mem and Write to mem each
     * just increase the access to mem.
     * @param address
     */
    void BringFromMem(string address);
    void WriteToMem(string address);

};


#endif //HW2ARCHFINAL_CACHESIM_H



