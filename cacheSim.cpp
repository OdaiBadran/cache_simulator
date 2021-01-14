/**
 * @author Odai Badran
 */

#include "cacheSim.h"
using namespace std;


/**
 * helper functions : power,log2,reverse
 */

static int power(int x, unsigned int y)
{
    if (y == 0)
        return 1;
    else if (y%2 == 0)
        return power(x, y/2)*power(x, y/2);
    else
        return x*power(x, y/2)*power(x, y/2);
}

static int log2(int x){
    int val = 1;
    int count = 0;
    while(val != x){
        val *= 2;
        ++count;
    }
    return count;
}

static void reverseStr(string& str)
{
    int n = str.length();

    // Swap character starting from two
    // corners
    for (int i = 0; i < n / 2; i++)
        swap(str[i], str[n - i - 1]);
}


Cache::Cache (int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
              int hit = 0, int miss = 0) {
    blockSize_ = power(2, blockSize);
    cacheSize_ = power(2, cacheSize);
    writeAlloc_ = writeAlloc;
    assoc_ = power(2, assoc);
    cyc_ = cyc;
    hit_ = hit;
    miss_ = miss;
    int blockNum = (cacheSize_ / blockSize_);
    dirty_ = new bool[blockNum];
    valid_ = new bool[blockNum];
    cache_ = new string[blockNum];
    tag_ = new int[blockNum];
    lru_ = new int[blockNum];
    initializeCache();
}
Cache::~Cache() {
    delete []dirty_;
    delete []valid_;
    delete []tag_;
    delete []lru_;
    delete []cache_;
    //for (int i = 0; i < cacheSize_/blockSize_; i++)
}

void Cache::initializeCache(){
    int blockNum = cacheSize_/blockSize_;
    for (int i = 0; i < blockNum; i++) {
        dirty_[i] = false;
        valid_[i] = false;
        lru_[i] = 0;
    }

}

int Cache::getSet(string& address) {
    //string binAddress = hexStrToBinStr(address);
    stringstream s;
    s << std::hex << address;
    int n_;
    s >> n_;
    bitset<32> b(n_);
    string binAddress = b.to_string();
    reverseStr(binAddress);
    int startBit = log2(blockSize_);
    int len = log2((cacheSize_/blockSize_)/assoc_);
    // check if there are no bits for set ==> set = 0
    if(len == 0) return 0;
    string set = binAddress.substr(startBit,len);
    reverseStr(set);
    return stoi(set, 0, 2);
}


int Cache::getTag(string& address) {
    stringstream s;
    s << std::hex << address;
    int n_;
    s >> n_;
    bitset<32> b(n_);
    string binAddress = b.to_string();
    reverseStr(binAddress);
    int startBit = log2(blockSize_) + log2((cacheSize_/blockSize_)/assoc_);
    int len = MAX_BITS - startBit;
    // check if there are no bits for tag ==> tag = 0
    if(len == 0) return 0;
    string tag = binAddress.substr(startBit,len);
    reverseStr(tag);
    return stoi(tag, 0, 2);
}
void Cache::updateLru(int set, int prior) {
    int prevPriority = lru_[prior];
    lru_[prior] = assoc_ - 1;
    int waySize = (cacheSize_/blockSize_)/assoc_;
    for (int i = 0; i < assoc_; i++) {
        int before = lru_[set + waySize * i];
        lru_[set + waySize * i] = ((lru_[set + waySize * i] > prevPriority)
                                   && ((set + waySize * i) != prior)) ?
                                  (lru_[set + waySize * i] - 1) : lru_[set + waySize * i];
        int after = lru_[set + waySize * i];
        int x = 0;
    }
}


CacheResult Cache::Search(string& address){
    int set = getSet(address);
    int tag = getTag(address);
    int waySize = (cacheSize_/blockSize_)/assoc_;
    for (int i = 0; i < assoc_; i++) {
        if (!valid_[set + waySize * i]) continue;
        if (tag_[set + waySize * i] == tag) { // address been found :
            ++hit_;
            updateLru(set,set + waySize*i);
            return HIT;
        }
    }
    ++miss_;
    return MISS;
}



L1::L1(int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
       int hit = 0, int miss = 0) : Cache(blockSize,cacheSize,writeAlloc,assoc,cyc){
}

void L1::snoop(string& address, L2& l2) {
    int set = getSet(address);
    int tag = getTag(address);
    int waySize = (cacheSize_/blockSize_)/assoc_;

    for (int i = 0; i < assoc_; i++) {
        if (!valid_[set + waySize * i]) continue;
        if (tag_[set + waySize * i] == tag) { // address been found :
            // if the suitable block is not dirty then simply invalidate it
            if (!dirty_[set + waySize * i]) {
                valid_[set + waySize * i] = false;
                //updateLru(set, set + waySize * i); // i think no need
                return;
            } else { // block is dirty write it back to L2 :
                // write this block back to L2
                l2.writeDirtyBlockToL2(cache_[set + waySize * i]);
                valid_[set + waySize * i] = false;
                dirty_[set + waySize * i] = false;
                //update(set, set + waySize * i); // i think no need
                return;
            }
        }
    }
    // otherwise block is in L2 not in L1 (no problem with that)
}



void L1::writeToL1(string& address, L2& l2, bool isHitInL1, char mode = 'w') {
    int set = getSet(address);
    int tag = getTag(address);
    int waySize = (cacheSize_/blockSize_)/ assoc_;

    if (!isHitInL1) {
        // look for the block with the lowest priority :
        for (int i = 0; i < assoc_; i++) {
            if (lru_[set + waySize * i] == 0) {
                if (dirty_[set + waySize * i]) {
                    l2.writeDirtyBlockToL2(cache_[set + waySize * i]);
                }
                tag_[set + waySize * i] = tag;
                cache_[set + waySize * i] = address;
                dirty_[set + waySize * i] = (mode != 'r');
                valid_[set + waySize * i] = true;
                updateLru(set, set + (waySize * i));
                return;
            }
        }

    } if (isHitInL1) {
        for (int i = 0; i < assoc_; i++) {
            if (!valid_[set + waySize * i]) continue;
            if (tag_[set + waySize * i] == tag) { // address been found :
                cache_[set + waySize * i] = address;
                dirty_[set + waySize * i] = true;
                valid_[set + waySize * i] = true;
                updateLru(set, set + waySize * i);
                return;
            }
        }
    }
    // should not get here, if so it means there is not block with 0 priority
    cout << "No Block With Zero Priority" << endl;
}

/**
 * L2 cache.
 * @author Odai Badran
 * @since 9/12/2020
 */

L2::L2(int blockSize, int cacheSize, int writeAlloc, int assoc, int cyc,
       int hit = 0, int miss = 0) : Cache(blockSize,cacheSize,writeAlloc,assoc,cyc){
}
void L2::writeToL2SnoopL1(string& address, L1& l1) {
    int set = getSet(address);
    int tag = getTag(address);
    int waySize = (cacheSize_/blockSize_)/ assoc_;

    for (int i = 0; i < assoc_; i++) {
        if (!valid_[set + waySize * i]) continue;
        if (tag == tag_[set +waySize*i]) {
            tag_[set + waySize * i] = tag;
            cache_[set + waySize * i] = address;
            dirty_[set + waySize * i] = true;
            valid_[set + waySize * i] = true;
            updateLru(set, set + waySize * i);
            return;
        }

    } // otherwise if all blocks suitable for this set are valid then*/
    // look for the block with the lowest priority :
    for (int i = 0; i < assoc_; i++) {
        if (lru_[set + waySize * i] == 0) {
            //string addr = l1.cache_[set + waySize * i];
            l1.snoop(cache_[set + waySize * i], *this);
            tag_[set + waySize * i] = tag;
            cache_[set + waySize * i] = address;
            dirty_[set + waySize * i] = true;
            valid_[set + waySize * i] = true;
            updateLru(set, set + waySize * i);
            return;
        }
    }
    // should not get here, if so it means there is not block with 0 priority
    cout << "No Block With Zero Priority" << endl;
}

void L2::writeDirtyBlockToL2(string& address) {
    int set = getSet(address);
    int tag = getTag(address);
    int waySize = (cacheSize_/blockSize_)/assoc_;

    for (int i = 0; i < assoc_; i++) {
        if (!valid_[set + waySize * i]) continue;
        if (tag_[set + waySize * i] == tag) { // address been found :
            // then write block of this address to L2 ..
            valid_[set + waySize * i] = true;
            dirty_[set + waySize * i] = true;
            updateLru(set, set + waySize * i);
            return;
        }
    }
    // should not get here since each block in L1 is in L2
    cout<<"Error!, block suitable for the address is in L1 not in L2"<<endl;
}



Mem::Mem(int cyc, int access = 0):cyc_(cyc),access_(access){}
void Mem::BringFromMem(string address) { ++access_;}
void Mem::WriteToMem(string address) { ++access_;}



int main(int argc, char** argv) {


    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments
    // File
    // Assuming it is the first argument
    char* fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    int MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
            L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = stoi(argv[i + 1]);
        } else if (s == "--bsize") {
            BSize = stoi(argv[i + 1]);
        } else if (s == "--l1-size") {
            L1Size = stoi(argv[i + 1]);
        } else if (s == "--l2-size") {
            L2Size = stoi(argv[i + 1]);
        } else if (s == "--l1-cyc") {
            L1Cyc = stoi(argv[i + 1]);
        } else if (s == "--l2-cyc") {
            L2Cyc = stoi(argv[i + 1]);
        } else if (s == "--l1-assoc") {
            L1Assoc = stoi(argv[i + 1]);
        } else if (s == "--l2-assoc") {
            L2Assoc = stoi(argv[i + 1]);
        } else if (s == "--wr-alloc") {
            WrAlloc = stoi(argv[i + 1]);
        } else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }



    // create L1, L2, Mem according to the above params
    L1 l1(BSize,L1Size,WrAlloc,L1Assoc,L1Cyc);

    L2 l2(BSize,L2Size,WrAlloc,L2Assoc,L2Cyc);

    Mem mem(MemCyc);


    while (getline(file, line)) {

        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;

        }
        if (operation == 'r') {

            if (l1.Search(address) == MISS) {

                if (l2.Search(address) == MISS) {
                    mem.BringFromMem(address);
                    l2.writeToL2SnoopL1(address, l1);
                    l1.writeToL1(address, l2, false, 'r');

                } else  // hit in l2, miss in L1
                    l1.writeToL1(address, l2, false, 'r');

            } else {/*hit in L1 ==> just read the block*/}
           // cout<<address<<" :";
           // print(l1,l2);
            continue;
        }

        if (operation == 'w') {

            // First case :
            // L1, L2 are using WriteAllocate methodology
            if (WrAlloc) {

                if (l1.Search(address) == MISS) {

                    if (l2.Search(address) == MISS) {
                        mem.BringFromMem(address);
                        l2.writeToL2SnoopL1(address, l1);
                        l1.writeToL1(address, l2, false, 'w');

                    } else  // hit in l2, miss in L1
                        l1.writeToL1(address, l2, false, 'w');

                } else  // hit in L1
                    l1.writeToL1(address, l2, true);
                //cout<<address<<" :";
                //print(l1,l2);
                continue;
            }

            // Second case :
            // L1, L2 are using NoWriteAllocate methodology
            if (!WrAlloc) {

                if (l1.Search(address) == MISS) {

                    if (l2.Search(address) == MISS) {
                        mem.WriteToMem(address);
                        continue;
                    } else  // hit in l2, miss in L1
                        l2.writeToL2SnoopL1(address, l1);

                } else  // hit in L1
                    l1.writeToL1(address, l2, true);
                //cout<<address<<" :";
                //print(l1,l2);
                continue;
            }


        } else { // should not get here, if so print Error
            cout << "Error!, WrAlloc Illegal Val " << endl;
            return -1;
        }



/*
		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;
*/
    }
    //cout<<mem.access_<< "<==access"<<endl;
    //cout<<l1.miss_<<endl; cout<<l1.hit_<<endl;
    //cout<<l2.miss_<<endl; cout<<l2.hit_<<endl;

    double L1MissRate = (double)(l1.miss_)/(l1.miss_+l1.hit_);
    double L2MissRate = (double)(l2.miss_)/(l2.miss_+l2.hit_);
    double avgAccTime = (double)(mem.access_*mem.cyc_ + (l2.miss_+l2.hit_)*l2.cyc_
                                 + (l1.miss_+l1.hit_)*l1.cyc_)/(l1.hit_+l1.miss_);


    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    return 0;
}

