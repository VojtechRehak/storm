#ifndef STORM_STORAGE_BISIMULATION_PARTITION_H_
#define STORM_STORAGE_BISIMULATION_PARTITION_H_

#include <cstddef>
#include <list>

#include "src/storage/bisimulation/Block.h"

#include "src/storage/BitVector.h"

namespace storm {
    namespace storage {
        namespace bisimulation {
            
            class Partition {
            public:
                /*!
                 * Creates a partition with one block consisting of all the states.
                 *
                 * @param numberOfStates The number of states the partition holds.
                 */
                Partition(std::size_t numberOfStates);
                
                /*!
                 * Creates a partition with three blocks: one with all phi states, one with all psi states and one with
                 * all other states. The former two blocks are marked as being absorbing, because their outgoing
                 * transitions shall not be taken into account for future refinement.
                 *
                 * @param numberOfStates The number of states the partition holds.
                 * @param prob0States The states which have probability 0 of satisfying phi until psi.
                 * @param prob1States The states which have probability 1 of satisfying phi until psi.
                 * @param representativeProb1State If the set of prob1States is non-empty, this needs to be a state
                 * that is representative for this block in the sense that the state representing this block in the quotient
                 * model will receive exactly the atomic propositions of the representative state.
                 */
                Partition(std::size_t numberOfStates, storm::storage::BitVector const& prob0States, storm::storage::BitVector const& prob1States, boost::optional<storm::storage::sparse::state_type> representativeProb1State);
                
                Partition() = default;
                Partition(Partition const& other) = default;
                Partition& operator=(Partition const& other) = default;
                Partition(Partition&& other) = default;
                Partition& operator=(Partition&& other) = default;
                
                // Retrieves the size of the partition, i.e. the number of blocks.
                std::size_t size() const;
                
                // Prints the partition to the standard output.
                void print() const;
                
                // Splits the block at the given position and inserts a new block after the current one holding the rest
                // of the states.
                std::pair<std::vector<std::unique_ptr<Block>>::iterator, bool> splitBlock(Block& block, storm::storage::sparse::state_type position);

                // Splits the block by sorting the states according to the given function and then identifying the split
                // points. The callback function is called for every newly created block.
                bool splitBlock(Block& block, std::function<bool (storm::storage::sparse::state_type, storm::storage::sparse::state_type)> const& less, std::function<void (Block&)> const& newBlockCallback = [] (Block&) {});

                // Splits all blocks by using the sorting-based splitting. The callback is called for all newly created
                // blocks.
                bool split(std::function<bool (storm::storage::sparse::state_type, storm::storage::sparse::state_type)> const& less, std::function<void (Block&)> const& newBlockCallback = [] (Block&) {});
                
                // Splits the block such that the resulting blocks contain only states in the given set or none at all.
                // If the block is split, the given block will contain the states *not* in the given set and the newly
                // created block will contain the states *in* the given set.
                void splitStates(Block& block, storm::storage::BitVector const& states);
                
                /*!
                 * Splits all blocks of the partition such that afterwards all blocks contain only states within the given
                 * set of states or no such state at all.
                 */
                void splitStates(storm::storage::BitVector const& states);
                
                // Sorts the block based on the state indices.
                void sortBlock(Block const& block);
                
                // Retrieves the blocks of the partition.
                std::vector<std::unique_ptr<Block>> const& getBlocks() const;
                
                // Retrieves the blocks of the partition.
                std::vector<std::unique_ptr<Block>>& getBlocks();
                
                // Checks the partition for internal consistency.
                bool check() const;
                
                // Returns an iterator to the beginning of the states of the given block.
                std::vector<storm::storage::sparse::state_type>::iterator begin(Block const& block);

                // Returns an iterator to the beginning of the states of the given block.
                std::vector<storm::storage::sparse::state_type>::const_iterator begin(Block const& block) const;

                // Returns an iterator to the beginning of the states of the given block.
                std::vector<storm::storage::sparse::state_type>::iterator end(Block const& block);
                
                // Returns an iterator to the beginning of the states of the given block.
                std::vector<storm::storage::sparse::state_type>::const_iterator end(Block const& block) const;
                
                // Swaps the positions of the two given states.
                void swapStates(storm::storage::sparse::state_type state1, storm::storage::sparse::state_type state2);
                
                // Retrieves the block of the given state.
                Block& getBlock(storm::storage::sparse::state_type state);
                
                // Retrieves the block of the given state.
                Block const& getBlock(storm::storage::sparse::state_type state) const;
                
                // Retrieves the position of the given state.
                storm::storage::sparse::state_type const& getPosition(storm::storage::sparse::state_type state) const;
                
                // Sets the position of the state to the given position.
                storm::storage::sparse::state_type const& getState(storm::storage::sparse::state_type position) const;
                
                // Updates the block mapping for the given range of states to the specified block.
                void mapStatesToBlock(Block& block, std::vector<storm::storage::sparse::state_type>::iterator first, std::vector<storm::storage::sparse::state_type>::iterator last);
                
                // Update the state to position for the states in the given block.
                void mapStatesToPositions(Block const& block);
                
            private:
                // FIXME: necessary?
                // Swaps the positions of the two states given by their positions.
                void swapStatesAtPositions(storm::storage::sparse::state_type position1, storm::storage::sparse::state_type position2);
                
                // FIXME: necessary?
                // Inserts a block before the given block. The new block will cover all states between the beginning
                // of the given block and the end of the previous block.
                Block& insertBlock(Block& block);
                
                // FIXME: necessary?
                // Sets the position of the given state.
                void setPosition(storm::storage::sparse::state_type state, storm::storage::sparse::state_type position);
                
                // The of blocks in the partition.
                std::vector<std::unique_ptr<Block>> blocks;
                
                // A mapping of states to their blocks.
                std::vector<Block*> stateToBlockMapping;
                
                // A vector containing all the states. It is ordered in a way such that the blocks only need to define
                // their start/end indices.
                std::vector<storm::storage::sparse::state_type> states;
                
                // This vector keeps track of the position of each state in the state vector.
                std::vector<storm::storage::sparse::state_type> positions;
            };
        }
    }
}

#endif /* STORM_STORAGE_BISIMULATION_PARTITION_H_ */