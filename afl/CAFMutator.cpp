#include <cstddef>
#include <cstdint>


#include "CAFMutator.hpp"


extern "C" {

size_t afl_custom_mutator(uint8_t *data, size_t size, uint8_t *mutated_out,
                          size_t max_size, unsigned int seed) {
  // TODO: Implement afl_custom_mutator.
}

size_t afl_pre_save_handler(uint8_t *data, size_t size, uint8_t **new_data) {
  // TODO: Implement afl_pre_save_handler.
}

}