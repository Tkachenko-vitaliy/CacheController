# Page cache controller:  implemention of page caching  

**Contents**  
- [The technology](#the-technology)
- [Operations](#operations)
- [How to use it](#how-to-use-it)
- [Cache policy](#cache-policy)
- [Cache algorithm](#cache-algorithm)
- [Page locator](#page-locator)
- [Cache statistic](#cache-statistic)


### The technology
Page caching is used mainly for the acceleration of access to the secondary storage (such as HDD, SSD, and others). Initially, a memory buffer is allocated in RAM (*cache memory*). All storage address space is divided into logical parts that are called *pages*. When application reads/writes data from/to the storage, a corresponding page is loaded to the cache memory. Further, if the application requires that data again, it operates not with the secondary storage, but with a copy of data in the cache memory.

### Operations
When the application needs to access data presumed to exist in the backing store, the cache controller checks if data pages are loaded in the cache. If so (cache hit), the copy of data in cache memory is used instead. The alternative situation, when the cache is checked and the corresponding page is not found (cache miss), the requested data is retrieved from the secondary storage and copied into the cache, ready for the next access.

During a cache miss, it can occur that there is no free pages cache memory. In that case, some pages are removed from the cache in order to load the newly retrieved data (page replacement). The rules, according to the pages have to be replaced are chosen, are define by the used cache algorithm.

### How to use it
The source code is located in the `source` directory. The directory `test` contains a set of tests that were used for testing of the components. Most probably, you will not have to use them.

The main class is **PageCacheController**. You should inherit that class and override methods **readStorage** and **writeStorage**, in which you should read/write data from the secondary storage. An example you can see in tests, ReadWrite.cpp

Before using, you should allocate cache memory by calling the **setupPages** method, in which you should assign page count and page size (in bytes). For the read/write data, call the appropriate **read** or **write** method. To force writing changed data to storage, call the **flush** method. 'metadata' is a user-defined variable; the controller does not process that parameter.

To enable/disable caching, use the **enable** method. If caching is not enabled, all read/write operations will operate directly with the secondary storage, bypass the cache.

To clear cache memory, use the **clear** method. If you want the cache memory page to be clear before loading, set the appropriate value in the **setCleanBeforeLoad** method.

Sometimes it is advisable that data pages are loaded not from the start address of the secondary storage (for example, file with header). You can set offset for the start address of the page by calling the **setStartPageOffset** method.

Multithread access and exception processing is supported; all read/write/flush operations can be performed independently in different threads.

if **readStorage** or **writeStorage** method throws the exception, the controller will retrow it to the all threads that wait access to the corresponding page.

### Cache policy
The data stored in cache memory at some point must be written to the storage as well. The timing of this write is controlled by the write policy:

*Write-through*: write is done synchronously both to the cache and to the storage.

*Write-back*: initially, writing is done only to the cache. The write to the storage is postponed until the modified content is about to be replaced by another cache block.

Default policy is Write-back. You can set write policy by calling the **setWritePolicy** method.

Since no data is returned to the requester on the write operations (cache miss), a decision needs to be made on write misses, whether or not data would be loaded into the cache. This is defined by the write miss policy:

*Write-allocate*: data at the missed-write location is loaded to cache, followed by a write-hit operation. In this approach, write misses are similar to read misses.

*Write-around*: data at the missed-write location is not loaded to cache, and is written directly to the storage. In this approach, data is loaded into the cache on read misses only.

Default policy is Write-allocate. You can set write miss policy by calling the **setWriteMissPolicy** method.

### Cache algorithm

If cache miss occurs, the cache algorithm defines rules what pages have to be replaced. The following algorithms were implemented:
- FIFO (First In, First Out);
-	LRU (Least Recently Used);
-	LFU (Least Frequently Used); 
-	MRU (Most Recently Used);
-	NRU (Not Recently Used);
-	Clock; 
-	Random.

Default algorithm is LRU. You can set cache algorithm by calling **setReplaceAlgoritm** method.

If you want to implement some other algorithm, you should do the following:
1)	Create class inherited *CacheAlgorithm*;
2)	Implement virtual methods;
3)	Add algorithm name to the ReplaceAlgoritm enumeration; 
4)	Add a code for creation of your class into *CacheAlgorithm::create method*.

### Page locator
When a controller executes a read/write operation, it figures out whether the required page is located in the cache. By default, that information is stored in the hash map, where for every number of page a sign is kept that points whether the page was loaded. The access to the information is very fast: O(1). 

The size of hash map increases dynamically in the process of page requirements. Thus, if the address space of the secondary storage is large, a size of hash map can be significant. If cache memory is small, to keep the whole hash map can be too excessive. Instead of hash map, you can use binary tree. In the binary tree, the information only about the loaded pages is stored. However, an access to the information in that access will be slower: O(log N).

To set page locator type, use the **setLocatorType** method.

You can limit a size of hash map memory by calling the **setLimitHashMemory** method. If the limit is set (more then 0, 0 means the size is not limited), the controller throws the exception if a memory size consumed by hash map exceed the specified value.

### Cache statistic

During operation, the controller gathers statistic information. You can retrieve that information by calling the **getStatistic** method. The information contains the following:

*operationCount* – common number of read/write operations;

*hitCount* – a number of cache hits;

*missCount* – a number of cache miss;

*directCount* – a number of operations of direct access to the storage. Direct access can occur if during cache miss processing all pages are in the ‘load’ state;

*locatorMemory* – the size of memory to be allocated for the page locator. Notice that for the binary tree locator the information is approximate, because it depends on the details of tree implementation in the STL container.

To reset cache statistic information, use the **resetStatistic** method. 

### P.S.
If you have questions or suggestions, write me to Tkachenko_vitaliy@hotmail.com
