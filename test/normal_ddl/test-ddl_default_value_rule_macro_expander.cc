#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <uc_string.h>

#include <ddl_column_rules.h>
#include <ddl_default_value_rule_macro.h>

using namespace std;
using namespace universals;
using namespace normal_ddl;

TEST(ddl_default_value_rule_macro_expander, test1) {
  ddl_default_value_rules default_value_rules{};
  const string macro_lhs{"macro_lhs"};
  const string macro_args_text{"(hello, there)"};
  const string macro_text{macro_lhs + macro_args_text};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro_lhs}, ddl_default_value_rule{"macro_rhs", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(0), string{"rewrite"}}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  try {
    macro_expander.expand_macro(macro_text);
    FAIL() << "expected thrown invalid_argument; nothing caught" << endl;
  } catch (const invalid_argument& e) {
    EXPECT_EQ(0, string{e.what()}.find(ddl_default_value_rule_macro_expander::make_malformed_text_msg(macro_lhs, macro_args_text, 0)));
  } catch (...) {
    FAIL() << "expected thrown invalid_argument; another caught" << endl;
  }
}

TEST(ddl_default_value_rule_macro_expander, test2) {
  ddl_default_value_rules default_value_rules{};
  const string macro_lhs{"macro_lhs"};
  const string macro_args_text{"(hello, there)"};
  const string macro_text{macro_lhs + macro_args_text};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro_lhs}, ddl_default_value_rule{"macro_rhs", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(1), string{"()"}}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  try {
    macro_expander.expand_macro(macro_text);
    FAIL() << "expected thrown invalid_argument; nothing caught" << endl;
  } catch (const invalid_argument& e) {
    EXPECT_EQ(0, string{e.what()}.find(ddl_default_value_rule_macro_expander::make_argcnt_mismatch_msg(macro_lhs, macro_args_text, 1, 2)));
  } catch (...) {
    FAIL() << "expected thrown invalid_argument; another caught" << endl;
  }
}

TEST(ddl_default_value_rule_macro_expander, test3) {
  ddl_default_value_rules default_value_rules{};
  const string macro_lhs{"macro_lhs"};
  const string macro_args_text{"(hello, there)"};
  const string macro_text{macro_lhs + macro_args_text};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro_lhs}, ddl_default_value_rule{"macro_rhs", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(2), string{"($2, sugar_bear, $1)"}}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  EXPECT_EQ("macro_rhs(there, sugar_bear, hello)", *macro_expander.expand_macro(macro_text));
  EXPECT_EQ("macro_rhs(there, sugar_bear, hello)", *macro_expander.expand_all_macros(macro_text));
}

TEST(ddl_default_value_rule_macro_expander, test4) {
  ddl_default_value_rules default_value_rules{};
  const string macro1_lhs{"macro1_lhs"};
  const string macro2_lhs{"macro2_lhs"};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro1_lhs}, ddl_default_value_rule{"macro1_rhs", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(3), string{"($3, macro2_lhs(sugar, bear), $1)"}}}}}).second);
  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro2_lhs}, ddl_default_value_rule{"", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(2), string{"($1_$2)"}}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  EXPECT_EQ("macro1_rhs(there, macro2_lhs(sugar, bear), hello)", *macro_expander.expand_macro("macro1_lhs(hello, macro2_lhs(sugar, bear), there)"));
  EXPECT_EQ("macro1_rhs(there, (sugar_bear), hello)", *macro_expander.expand_all_macros("macro1_lhs(hello, macro2_lhs(sugar, bear), there)"));
}

TEST(ddl_default_value_rule_macro_expander, test5) {
  ddl_default_value_rules default_value_rules{};
  const string macro_lhs{"getdate"};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro_lhs}, ddl_default_value_rule{"now", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(0)}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  EXPECT_EQ("now", *macro_expander.expand_macro("getdate()"));
  EXPECT_EQ("now", *macro_expander.expand_all_macros("getdate()"));
}

TEST(ddl_default_value_rule_macro_expander, test6) {
  ddl_default_value_rules default_value_rules{};
  const string macro_lhs{"nothing"};

  EXPECT_EQ(true, default_value_rules.insert(pair<uc_string, ddl_default_value_rule>{uc_string{macro_lhs}, ddl_default_value_rule{"", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(0)}}}}).second);
  ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
  EXPECT_EQ("", *macro_expander.expand_macro("nothing"));
  EXPECT_EQ("", *macro_expander.expand_all_macros("nothing"));
}