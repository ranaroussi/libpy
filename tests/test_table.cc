#include <algorithm>
#include <array>
#include <ostream>
#include <vector>

#include "gtest/gtest.h"

#include "libpy/char_sequence.h"
#include "libpy/table.h"

namespace test_table {
using namespace py::cs::literals;

/** A non-fundamental type.
 */
class custom_object {
public:
    int a;
    float b;

    custom_object() = delete;
    explicit custom_object(int a) : a(a), b(a / 2.0){};

    bool operator==(const custom_object& other) const {
        return a == other.a && b == other.b;
    }

    custom_object& operator++() {
        ++a;
        ++b;
        return *this;
    }
};

std::ostream& operator<<(std::ostream& s, const custom_object& ob) {
    return s << "<custom_object a=" << ob.a << ", b=" << ob.b << '>';
}

TEST(row_view, assign) {
    using R = py::row_view<py::C<std::int64_t>("a"_cs),
                           py::C<double>("b"_cs),
                           py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row_view(&a, &b, &c);

    EXPECT_EQ(row_view.get("a"_cs), 1L);
    EXPECT_EQ(row_view.get("b"_cs), 2.5);
    EXPECT_EQ(row_view.get("c"_cs), custom_object(3));

    // assign through the view
    row_view = std::make_tuple(2, 3.5, custom_object(4));

    EXPECT_EQ(row_view.get("a"_cs), 2L);
    EXPECT_EQ(a, 2l);
    EXPECT_EQ(row_view.get("b"_cs), 3.5);
    EXPECT_EQ(b, 3.5);
    EXPECT_EQ(row_view.get("c"_cs), custom_object(4));
    EXPECT_EQ(c, custom_object(4));

    // assign to the underlying
    a = 3;
    b = 4.5;
    c = custom_object(5);

    EXPECT_EQ(row_view.get("a"_cs), 3L);
    EXPECT_EQ(row_view.get("b"_cs), 4.5);
    EXPECT_EQ(row_view.get("c"_cs), custom_object(5));
}

TEST(row_view, structured_binding) {
    using R = py::row_view<py::C<std::int64_t>("a"_cs),
                           py::C<double>("b"_cs),
                           py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row_view(&a, &b, &c);

    auto [bound_a, bound_b, bound_c] = row_view;
    EXPECT_EQ(bound_a, a);
    EXPECT_EQ(bound_b, b);
    EXPECT_EQ(bound_c, c);

    auto& [ref_a, ref_b, ref_c] = row_view;
    EXPECT_EQ(ref_a, a);
    EXPECT_EQ(ref_b, b);
    EXPECT_EQ(ref_c, c);

    ref_a = 2;
    ref_b = 3.5;
    ref_c = custom_object(4);

    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 3.5);
    EXPECT_EQ(c, custom_object(4));
}

TEST(row_view, subset) {
    using R = py::row_view<py::C<std::int64_t>("a"_cs),
                           py::C<double>("b"_cs),
                           py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row_view(&a, &b, &c);

    {
        // drop the `c` column
        auto subset = row_view.subset("a"_cs, "b"_cs);
        EXPECT_EQ(subset, std::make_tuple(a, b));
    }

    {
        // transpose columns
        auto subset = row_view.subset("b"_cs, "a"_cs, "c"_cs);
        EXPECT_EQ(subset, std::make_tuple(b, a, c));
    }

    {
        // mutate through subset
        auto subset = row_view.subset("a"_cs, "b"_cs);
        subset = std::make_tuple(2, 3.5);
        EXPECT_EQ(subset, std::make_tuple(2, 3.5));
        EXPECT_EQ(row_view, std::make_tuple(2, 3.5, custom_object(3)));
    }
}

TEST(row_view, drop) {
    using R = py::row_view<py::C<std::int64_t>("a"_cs),
                           py::C<double>("b"_cs),
                           py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row_view(&a, &b, &c);

    {
        // drop the `c` column
        auto subset = row_view.drop("c"_cs);
        EXPECT_EQ(subset, std::make_tuple(a, b));
    }

    {
        // drop 2 columns
        auto subset = row_view.drop("a"_cs, "c"_cs);
        EXPECT_EQ(subset, std::make_tuple(b));
    }

    {
        // mutate through subset
        auto subset = row_view.drop("b"_cs);
        subset = std::make_tuple(2, custom_object(4));
        EXPECT_EQ(subset, std::make_tuple(2, custom_object(4)));
        EXPECT_EQ(row_view, std::make_tuple(2, 2.5, custom_object(4)));
    }
}

TEST(row_view, relabel) {
    using R = py::row_view<py::C<std::int64_t>("a"_cs),
                           py::C<double>("b"_cs),
                           py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row_view(&a, &b, &c);

    auto relabeled = row_view.relabel(std::make_pair("a"_cs, "a-new"_cs),
                                      std::make_pair("c"_cs, "c-new"_cs));

    EXPECT_EQ(&relabeled.get("a-new"_cs), &row_view.get("a"_cs));
    EXPECT_EQ(&relabeled.get("b"_cs), &row_view.get("b"_cs));
    EXPECT_EQ(&relabeled.get("c-new"_cs), &row_view.get("c"_cs));
}

TEST(row, assign) {
    using R = py::row<py::C<std::int64_t>("a"_cs),
                      py::C<double>("b"_cs),
                      py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row(a, b, c);

    auto expect_original_unchanged = [&]() {
        EXPECT_EQ(a, 1L);
        EXPECT_EQ(b, 2.5);
        EXPECT_EQ(c, custom_object(3));
    };

    EXPECT_EQ(row.get("a"_cs), 1L);
    EXPECT_EQ(row.get("b"_cs), 2.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(3));

    // assign with a tuple
    row = std::make_tuple(2, 3.5, custom_object(4));

    EXPECT_EQ(row.get("a"_cs), 2L);
    EXPECT_EQ(row.get("b"_cs), 3.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(4));
    expect_original_unchanged();

    // assign with another row
    R new_row(3, 4.5, custom_object(5));
    row = new_row;
    EXPECT_EQ(row, new_row);
    EXPECT_EQ(row.get("a"_cs), 3L);
    EXPECT_EQ(row.get("b"_cs), 4.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(5));

    // assign with a view
    a = 4;
    b = 5.5;
    c = custom_object(6);

    using RV = py::row_view<py::C<std::int64_t>("a"_cs),
                            py::C<double>("b"_cs),
                            py::C<custom_object>("c"_cs)>;

    RV row_view(&a, &b, &c);

    row = row_view;
    EXPECT_EQ(row, row_view);
    EXPECT_EQ(row.get("a"_cs), 4L);
    EXPECT_EQ(row.get("b"_cs), 5.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(6));

    // assign to the underlying objects of the view;
    a = 5;
    b = 6.5;
    c = custom_object(7);

    row = row_view;
    EXPECT_EQ(row, row_view);
    EXPECT_EQ(row.get("a"_cs), 5L);
    EXPECT_EQ(row.get("b"_cs), 6.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(7));
}

TEST(row, structured_binding) {
    using R = py::row<py::C<std::int64_t>("a"_cs),
                      py::C<double>("b"_cs),
                      py::C<custom_object>("c"_cs)>;

    std::int64_t a = 1;
    double b = 2.5;
    custom_object c(3);
    R row(a, b, c);

    auto [bound_a, bound_b, bound_c] = row;
    EXPECT_EQ(bound_a, a);
    EXPECT_EQ(bound_b, b);
    EXPECT_EQ(bound_c, c);

    auto& [ref_a, ref_b, ref_c] = row;
    EXPECT_EQ(ref_a, a);
    EXPECT_EQ(ref_b, b);
    EXPECT_EQ(ref_c, c);

    // these references are into the row, not the original variables
    ref_a = 2;
    ref_b = 3.5;
    ref_c = custom_object(4);

    EXPECT_EQ(row.get("a"_cs), 2L);
    EXPECT_EQ(row.get("b"_cs), 3.5);
    EXPECT_EQ(row.get("c"_cs), custom_object(4));

    // the original values are unchanged
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2.5);
    EXPECT_EQ(c, custom_object(3));
}

TEST(row, cat) {
    using A =
        py::row<py::C<std::int64_t>("a_first"_cs), py::C<std::int32_t>("a_second"_cs)>;
    using B = py::row<py::C<double>("b_first"_cs), py::C<float>("b_second"_cs)>;
    using C = py::row<py::C<std::string_view>("c_first"_cs),
                      py::C<std::string>("c_second"_cs),
                      py::C<std::string_view>("c_third"_cs),
                      py::C<std::string_view>("c_fourth"_cs)>;

    A a(1, 2);
    B b(3.5, 4.5);
    C c("l", "m", "a", "o");

    auto actual_first_cat = py::row_cat(a, b);

    using first_cat_type = py::row<py::C<std::int64_t>("a_first"_cs),
                                   py::C<std::int32_t>("a_second"_cs),
                                   py::C<double>("b_first"_cs),
                                   py::C<float>("b_second"_cs)>;
    EXPECT_TRUE((std::is_same_v<decltype(actual_first_cat), first_cat_type>) );

    first_cat_type expected_first_cat(1, 2, 3.5, 4.5);
    EXPECT_EQ(actual_first_cat, expected_first_cat);

    auto actual_second_cat = py::row_cat(a, b, c);

    using second_cat_type = py::row<py::C<std::int64_t>("a_first"_cs),
                                    py::C<std::int32_t>("a_second"_cs),
                                    py::C<double>("b_first"_cs),
                                    py::C<float>("b_second"_cs),
                                    py::C<std::string_view>("c_first"_cs),
                                    py::C<std::string>("c_second"_cs),
                                    py::C<std::string_view>("c_third"_cs),
                                    py::C<std::string_view>("c_fourth"_cs)>;
    EXPECT_TRUE((std::is_same_v<decltype(actual_second_cat), second_cat_type>) );

    second_cat_type expected_second_cat(1, 2, 3.5, 4.5, "l", "m", "a", "o");
    EXPECT_EQ(actual_second_cat, expected_second_cat);
}

TEST(table, emplace_back) {
    using T = py::table<py::C<std::int64_t>("a"_cs),
                        py::C<double>("b"_cs),
                        py::C<custom_object>("c"_cs)>;
    T table;
    ASSERT_EQ(table.size(), 0ul);

    // insert a tuple
    auto expected_row_0 = std::make_tuple(1, 2.5, custom_object(3));
    table.emplace_back(expected_row_0);
    ASSERT_EQ(table.size(), 1ul);

    auto test_row_0 = [&]() {
        auto row = table.rows()[0];
        EXPECT_EQ(row, row);
        EXPECT_EQ(row, expected_row_0);
        EXPECT_EQ(row, row.copy());
    };

    test_row_0();

    // insert a row
    auto expected_row_1 = T::row_type(2, 3.5, custom_object(4));
    table.emplace_back(expected_row_1);
    ASSERT_EQ(table.size(), 2ul);

    auto test_row_1 = [&]() {
        auto row = table.rows()[1];
        EXPECT_EQ(row, row);
        EXPECT_EQ(row, expected_row_1);
        EXPECT_EQ(row, row.copy());
    };

    test_row_0();
    test_row_1();

    // insert a row_view
    std::int64_t a = 3;
    double b = 4.5;
    custom_object c(5);
    auto expected_row_2 = T::row_view_type(&a, &b, &c);
    table.emplace_back(expected_row_2);
    ASSERT_EQ(table.size(), 3ul);

    test_row_0();
    test_row_1();

    {
        auto row = table.rows()[2];
        EXPECT_EQ(row, row);
        EXPECT_EQ(row, expected_row_2);
        EXPECT_EQ(row, row.copy());
    }
}

TEST(table, row_iter) {
    using T = py::table<py::C<std::int64_t>("a"_cs),
                        py::C<double>("b"_cs),
                        py::C<custom_object>("c"_cs)>;

    T table;

    std::int64_t a = 0;
    double b = 1.5;
    custom_object c(2);
    for (std::size_t ix = 0; ix < 64; ++ix) {
        table.emplace_back(std::make_tuple(++a, ++b, ++c));
    }

    std::int64_t expected_a = 0;
    double expected_b = 1.5;
    custom_object expected_c(2);

    for (auto row : table.rows()) {
        EXPECT_EQ(row.get("a"_cs), ++expected_a);
        EXPECT_EQ(row.get("b"_cs), ++expected_b);
        EXPECT_EQ(row.get("c"_cs), ++expected_c);
    }
}

TEST(table_view, relabel) {
    using Table = py::table<py::C<std::int64_t>("a"_cs),
                            py::C<double>("b"_cs),
                            py::C<custom_object>("c"_cs)>;
    using View = typename Table::view_type;

    Table table;

    std::int64_t a = 0;
    double b = 1.5;
    custom_object c(2);
    for (std::size_t ix = 0; ix < 64; ++ix) {
        table.emplace_back(std::make_tuple(++a, ++b, ++c));
    }

    View view(table);

    auto relabeled = view.relabel(std::make_pair("a"_cs, "a-new"_cs),
                                  std::make_pair("c"_cs, "c-new"_cs));

    ASSERT_EQ(relabeled.size(), view.size());
    ASSERT_EQ(relabeled.size(), 64ul);
    for (std::size_t ix = 0; ix < relabeled.size(); ++ix) {
        auto base_row = view.rows()[ix];
        auto relabeled_row = relabeled.rows()[ix];

        EXPECT_EQ(&relabeled_row.get("a-new"_cs), &base_row.get("a"_cs));
        EXPECT_EQ(&relabeled_row.get("b"_cs), &base_row.get("b"_cs));
        EXPECT_EQ(&relabeled_row.get("c-new"_cs), &base_row.get("c"_cs));
    }
}
}  // namespace test_table
