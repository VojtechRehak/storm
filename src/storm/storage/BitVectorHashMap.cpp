#include "storm/storage/BitVectorHashMap.h"

#include <algorithm>
#include <iostream>
#include <algorithm>

#include "storm/utility/macros.h"

namespace storm {
    namespace storage {
        template<class ValueType, class Hash1, class Hash2>
        const std::vector<std::size_t> BitVectorHashMap<ValueType, Hash1, Hash2>::sizes = {5, 13, 31, 79, 163, 277, 499, 1021, 2029, 3989, 8059, 16001, 32099, 64301, 127921, 256499, 511111, 1024901, 2048003, 4096891, 8192411, 15485863, 32142191, 64285127, 128572517, 257148523, 514299959, 102863003};
        
        template<class ValueType, class Hash1, class Hash2>
        BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::BitVectorHashMapIterator(BitVectorHashMap const& map, BitVector::const_iterator indexIt) : map(map), indexIt(indexIt) {
            // Intentionally left empty.
        }
        
        template<class ValueType, class Hash1, class Hash2>
        bool BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::operator==(BitVectorHashMapIterator const& other) {
            return &map == &other.map && *indexIt == *other.indexIt;
        }

        template<class ValueType, class Hash1, class Hash2>
        bool BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::operator!=(BitVectorHashMapIterator const& other) {
            return !(*this == other);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        typename BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator& BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::operator++(int) {
            ++indexIt;
            return *this;
        }

        template<class ValueType, class Hash1, class Hash2>
        typename BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator& BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::operator++() {
            ++indexIt;
            return *this;
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::pair<storm::storage::BitVector, ValueType> BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMapIterator::operator*() const {
            return map.getBucketAndValue(*indexIt);
        }
                
        template<class ValueType, class Hash1, class Hash2>
        BitVectorHashMap<ValueType, Hash1, Hash2>::BitVectorHashMap(uint64_t bucketSize, uint64_t initialSize, double loadFactor) : loadFactor(loadFactor), bucketSize(bucketSize), numberOfElements(0) {
            STORM_LOG_ASSERT(bucketSize % 64 == 0, "Bucket size must be a multiple of 64.");
            currentSizeIterator = std::find_if(sizes.begin(), sizes.end(), [=] (uint64_t value) { return value > initialSize; } );
            
            // Create the underlying containers.
            buckets = storm::storage::BitVector(bucketSize * *currentSizeIterator);
            occupied = storm::storage::BitVector(*currentSizeIterator);
            values = std::vector<ValueType>(*currentSizeIterator);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        bool BitVectorHashMap<ValueType, Hash1, Hash2>::isBucketOccupied(uint_fast64_t bucket) const {
            return occupied.get(bucket);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::size_t BitVectorHashMap<ValueType, Hash1, Hash2>::size() const {
            return numberOfElements;
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::size_t BitVectorHashMap<ValueType, Hash1, Hash2>::capacity() const {
            return *currentSizeIterator;
        }
        
        template<class ValueType, class Hash1, class Hash2>
        void BitVectorHashMap<ValueType, Hash1, Hash2>::increaseSize() {
            ++currentSizeIterator;
            STORM_LOG_ASSERT(currentSizeIterator != sizes.end(), "Hash map became to big.");
            
            // Create new containers and swap them with the old ones.
            numberOfElements = 0;
            storm::storage::BitVector oldBuckets(bucketSize * *currentSizeIterator);
            std::swap(oldBuckets, buckets);
            storm::storage::BitVector oldOccupied = storm::storage::BitVector(*currentSizeIterator);
            std::swap(oldOccupied, occupied);
            std::vector<ValueType> oldValues = std::vector<ValueType>(*currentSizeIterator);
            std::swap(oldValues, values);
            
            // Now iterate through the elements and reinsert them in the new storage.
            bool fail = false;
            for (auto bucketIndex : oldOccupied) {
                fail = !this->insertWithoutIncreasingSize(oldBuckets.get(bucketIndex * bucketSize, bucketSize), oldValues[bucketIndex]);
                
                // If we failed to insert just one element, we have to redo the procedure with a larger container size.
                if (fail) {
                    break;
                }
            }
            
            uint_fast64_t failCount = 0;
            while (fail) {
                ++failCount;
                STORM_LOG_ASSERT(failCount < 2, "Increasing size failed too often.");
                
                ++currentSizeIterator;
                STORM_LOG_ASSERT(currentSizeIterator != sizes.end(), "Hash map became to big.");
                
                numberOfElements = 0;
                buckets = storm::storage::BitVector(bucketSize * *currentSizeIterator);
                occupied  = storm::storage::BitVector(*currentSizeIterator);
                values = std::vector<ValueType>(*currentSizeIterator);
                
                for (auto bucketIndex : oldOccupied) {
                    fail = !this->insertWithoutIncreasingSize(oldBuckets.get(bucketIndex * bucketSize, bucketSize), oldValues[bucketIndex]);
                    
                    // If we failed to insert just one element, we have to redo the procedure with a larger container size.
                    if (fail) {
                        continue;
                    }
                }
            }
        }
        
        template<class ValueType, class Hash1, class Hash2>
        bool BitVectorHashMap<ValueType, Hash1, Hash2>::insertWithoutIncreasingSize(storm::storage::BitVector const& key, ValueType const& value) {
            std::tuple<bool, std::size_t, bool> flagBucketTuple = this->findBucketToInsert<false>(key);
            if (std::get<2>(flagBucketTuple)) {
                return false;
            }
            
            if (std::get<0>(flagBucketTuple)) {
                return true;
            } else {
                // Insert the new bits into the bucket.
                buckets.set(std::get<1>(flagBucketTuple) * bucketSize, key);
                occupied.set(std::get<1>(flagBucketTuple));
                values[std::get<1>(flagBucketTuple)] = value;
                ++numberOfElements;
                return true;
            }
        }
        
        template<class ValueType, class Hash1, class Hash2>
        ValueType BitVectorHashMap<ValueType, Hash1, Hash2>::findOrAdd(storm::storage::BitVector const& key, ValueType const& value) {
            return findOrAddAndGetBucket(key, value).first;
        }
        
        template<class ValueType, class Hash1, class Hash2>
        void BitVectorHashMap<ValueType, Hash1, Hash2>::setOrAdd(storm::storage::BitVector const& key, ValueType const& value) {
            setOrAddAndGetBucket(key, value);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::pair<ValueType, std::size_t> BitVectorHashMap<ValueType, Hash1, Hash2>::findOrAddAndGetBucket(storm::storage::BitVector const& key, ValueType const& value) {
            // If the load of the map is too high, we increase the size.
            if (numberOfElements >= loadFactor * *currentSizeIterator) {
                this->increaseSize();
            }
            
            std::tuple<bool, std::size_t, bool> flagBucketTuple = this->findBucketToInsert<true>(key);
            STORM_LOG_ASSERT(!std::get<2>(flagBucketTuple), "Failed to find bucket for insertion.");
            if (std::get<0>(flagBucketTuple)) {
                return std::make_pair(values[std::get<1>(flagBucketTuple)], std::get<1>(flagBucketTuple));
            } else {
                // Insert the new bits into the bucket.
                buckets.set(std::get<1>(flagBucketTuple) * bucketSize, key);
                occupied.set(std::get<1>(flagBucketTuple));
                values[std::get<1>(flagBucketTuple)] = value;
                ++numberOfElements;
                return std::make_pair(value, std::get<1>(flagBucketTuple));
            }
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::size_t BitVectorHashMap<ValueType, Hash1, Hash2>::setOrAddAndGetBucket(storm::storage::BitVector const& key, ValueType const& value) {
            // If the load of the map is too high, we increase the size.
            if (numberOfElements >= loadFactor * *currentSizeIterator) {
                this->increaseSize();
            }
            
            std::tuple<bool, std::size_t, bool> flagBucketTuple = this->findBucketToInsert<true>(key);
            STORM_LOG_ASSERT(!std::get<2>(flagBucketTuple), "Failed to find bucket for insertion.");
            if (!std::get<0>(flagBucketTuple)) {
                // Insert the new bits into the bucket.
                buckets.set(std::get<1>(flagBucketTuple) * bucketSize, key);
                occupied.set(std::get<1>(flagBucketTuple));
                ++numberOfElements;
            }
            values[std::get<1>(flagBucketTuple)] = value;
            return std::get<1>(flagBucketTuple);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        ValueType BitVectorHashMap<ValueType, Hash1, Hash2>::getValue(storm::storage::BitVector const& key) const {
            std::pair<bool, std::size_t> flagBucketPair = this->findBucket(key);
            STORM_LOG_ASSERT(flagBucketPair.first, "Unknown key.");
            return values[flagBucketPair.second];
        }
        
        template<class ValueType, class Hash1, class Hash2>
        bool BitVectorHashMap<ValueType, Hash1, Hash2>::contains(storm::storage::BitVector const& key) const {
            return findBucket(key).first;
        }

        template<class ValueType, class Hash1, class Hash2>
        typename BitVectorHashMap<ValueType, Hash1, Hash2>::const_iterator BitVectorHashMap<ValueType, Hash1, Hash2>::begin() const {
            return const_iterator(*this, occupied.begin());
        }
        
        template<class ValueType, class Hash1, class Hash2>
        typename BitVectorHashMap<ValueType, Hash1, Hash2>::const_iterator BitVectorHashMap<ValueType, Hash1, Hash2>::end() const {
            return const_iterator(*this, occupied.end());
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::pair<bool, std::size_t> BitVectorHashMap<ValueType, Hash1, Hash2>::findBucket(storm::storage::BitVector const& key) const {
            uint_fast64_t initialHash = hasher1(key) % *currentSizeIterator;
            uint_fast64_t bucket = initialHash;

            uint_fast64_t counter = 0;
            while (isBucketOccupied(bucket)) {
                ++counter;
                if (buckets.matches(bucket * bucketSize, key)) {
                    return std::make_pair(true, bucket);
                }
                bucket += hasher2(key);
                bucket %= *currentSizeIterator;
                
                if (bucket == initialHash) {
                    return std::make_pair(false, bucket);
                }
            }

            return std::make_pair(false, bucket);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        template<bool increaseStorage>
        std::tuple<bool, std::size_t, bool> BitVectorHashMap<ValueType, Hash1, Hash2>::findBucketToInsert(storm::storage::BitVector const& key) {
            uint_fast64_t initialHash = hasher1(key) % *currentSizeIterator;
            uint_fast64_t bucket = initialHash;
            
            uint_fast64_t counter = 0;
            while (isBucketOccupied(bucket)) {
                ++counter;
                if (buckets.matches(bucket * bucketSize, key)) {
                    return std::make_tuple(true, bucket, false);
                }
                bucket += hasher2(key);
                bucket %= *currentSizeIterator;
                
                if (bucket == initialHash) {
                    if (increaseStorage) {
                        this->increaseSize();
                        bucket = initialHash = hasher1(key) % *currentSizeIterator;
                    } else {
                        return std::make_tuple(false, bucket, true);
                    }
                }
            }
            
            return std::make_tuple(false, bucket, false);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        std::pair<storm::storage::BitVector, ValueType> BitVectorHashMap<ValueType, Hash1, Hash2>::getBucketAndValue(std::size_t bucket) const {
            return std::make_pair(buckets.get(bucket * bucketSize, bucketSize), values[bucket]);
        }
        
        template<class ValueType, class Hash1, class Hash2>
        void BitVectorHashMap<ValueType, Hash1, Hash2>::remap(std::function<ValueType(ValueType const&)> const& remapping) {
            for (auto pos : occupied) {
                values[pos] = remapping(values[pos]);
            }
        }
        
        template class BitVectorHashMap<uint_fast64_t>;
        template class BitVectorHashMap<uint32_t>;
    }
}
