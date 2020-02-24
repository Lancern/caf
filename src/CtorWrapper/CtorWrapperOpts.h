#ifndef CAF_CTOR_WRAPPER_OPTS_H
#define CAF_CTOR_WRAPPER_OPTS_H

namespace caf {

/**
 * @brief Provide options for the CAF constructor wrapper action.
 *
 */
struct CtorWrapperOpts {
  /**
   * @brief Should we dump AST after the wrapper action?
   *
   */
  bool DumpAST;
}; // struct CtorWrapperOpts

} // namespace caf

#endif
