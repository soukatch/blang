#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

struct symbol {
  symbol() {}
};

struct symbol_table final {
  symbol_table(symbol_table &prev) : prev_{prev} {}

  // We do not need any custom copy/move constructor/assignment operators right
  // now. Neither do we need to write a destructor.

  void put(auto &&s, auto &&sym)
    requires(std::is_same_v<
                 std::string,
                 std::remove_const_t<std::remove_reference_t<decltype(s)>>> &&
             std::is_same_v<
                 symbol,
                 std::remove_const_t<std::remove_reference_t<decltype(sym)>>>)
  {
    /// TODO: Should we do something with the return?
    table_.emplace(std::piecewise_construct, std::forward_as_tuple(s),
                   std::forward_as_tuple(sym));
  }

  // Need to use unique_ptr b/c reference won't work since references aren't
  // nullable, and I don't want to do something like std::pair<bool, symbol&>
  // since that woudl end up gettin pretty dirty :/
  //                    |
  //                    V

  [[nodiscard]] std::unique_ptr<symbol> get(auto &&s)
    requires(std::is_same_v<
             std::string,
             std::remove_const_t<std::remove_reference_t<decltype(s)>>>)
  {
    for (auto &&e{this}; e; e = &e->prev_)
      if (e->table_.contains(s))
        return std::make_unique(e->table_[s]);

    return nullptr;
  }

private:
  // & or * ... choices, choices.
  symbol_table &prev_;
  std::unordered_map<std::string, symbol> table_{};
};