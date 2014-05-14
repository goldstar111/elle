#include <deque>
#include <list>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <elle/attribute.hh>
#include <elle/container/deque.hh>
#include <elle/container/list.hh>
#include <elle/container/map.hh>
#include <elle/container/vector.hh>
#include <elle/serialization/json.hh>
#include <elle/serialization/json/MissingKey.hh>
#include <elle/serialization/json/TypeError.hh>
#include <elle/test.hh>

template <typename Format>
static
void
fundamentals()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    int i = -42;
    output.serialize("int", i);
    unsigned int ui = 42;
    output.serialize("unsigned int", ui);
    double d = 51.51;
    output.serialize("double", d);
    double round = 51.;
    output.serialize("round", round);
  }
  {
    typename Format::SerializerIn input(stream);
    int i = 0;
    input.serialize("int", i);
    BOOST_CHECK_EQUAL(i, -42);
    int ui = 0;
    input.serialize("unsigned int", ui);
    BOOST_CHECK_EQUAL(ui, 42);
    double d = 0;
    input.serialize("double", d);
    BOOST_CHECK_EQUAL(d, 51.51);
    double round = 0;
    input.serialize("round", round);
    BOOST_CHECK_EQUAL(round, 51.);
  }
}

class Point
{
public:
  Point()
    : _x(0)
    , _y(0)
  {}

  Point(int x, int y)
    : _x(x)
    , _y(y)
  {}

  Point(elle::serialization::Serializer& s)
    : Point()
  {
    this->serialize(s);
  }

  bool
  operator ==(Point const& other) const
  {
    return this->x() == other.x() && this->y() == other.y();
  }

  void
  serialize(elle::serialization::Serializer& s)
  {
    s.serialize("x", this->_x);
    s.serialize("y", this->_y);
  }

  ELLE_ATTRIBUTE_R(int, x);
  ELLE_ATTRIBUTE_R(int, y);
};

static
std::ostream&
operator << (std::ostream& out, Point const& p)
{
  out << "Point(" << p.x() << ", " << p.y() << ")";
  return out;
}

template <typename Format>
static
void
object()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    Point p(42, 51);
    p.serialize(output);
  }
  {
    typename Format::SerializerIn input(stream);
    Point p(input);
    BOOST_CHECK_EQUAL(p, Point(42, 51));
  }
}

class Line
{
public:
  Line()
    : _start()
    , _end()
  {}

  Line(Point start, Point end)
    : _start(start)
    , _end(end)
  {}

  Line(elle::serialization::Serializer& s)
    : Line()
  {
    this->serialize(s);
  }

  bool
  operator ==(Line const& other) const
  {
    return this->start() == other.start() && this->end() == other.end();
  }

  void
  serialize(elle::serialization::Serializer& s)
  {
    s.serialize("start", this->_start);
    s.serialize("end", this->_end);
  }

  ELLE_ATTRIBUTE_R(Point, start);
  ELLE_ATTRIBUTE_R(Point, end);
};

static
std::ostream&
operator << (std::ostream& out, Line const& l)
{
  out << "Line(" << l.start() << ", " << l.end() << ")";
  return out;
}

template <typename Format>
static
void
object_composite()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    Line l(Point(42, 51), Point(69, 86));
    l.serialize(output);
  }
  {
    typename Format::SerializerIn input(stream);
    Line l(input);
    BOOST_CHECK_EQUAL(l, Line(Point(42, 51), Point(69, 86)));
  }
}

template <template <typename, typename> class Container>
class Lists
{
public:
  Lists()
    : ints()
    , strings()
  {}

  Lists(elle::serialization::Serializer& s)
    : ints()
    , strings()
  {
    this->serialize(s);
  }

  void
  serialize(elle::serialization::Serializer& s)
  {
    s.serialize("ints", this->ints);
    s.serialize("strings", this->strings);
  }

  Container<int, std::allocator<int>> ints;
  Container<std::string, std::allocator<std::string>> strings;
};

template <typename Format, template <typename, typename> class Container>
static
void
array()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    Lists<Container> l;
    l.ints = {0, 1, 2};
    l.strings = {"foo", "bar", "baz"};
    l.serialize(output);
  }
  {
    typename Format::SerializerIn input(stream);
    Lists<Container> l(input);
    BOOST_CHECK_EQUAL(l.ints,
                      (Container<int, std::allocator<int>>{0, 1, 2}));
    BOOST_CHECK_EQUAL(l.strings,
                      (Container<std::string, std::allocator<std::string>>
                      {"foo", "bar", "baz"}));
  }
}

template <typename Format>
static
void
pair()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    std::pair<int, std::string> p(4, "foo");
    output.serialize("pair", p);
  }
  {
    typename Format::SerializerIn input(stream);
    std::pair<int, std::string> p;
    input.serialize("pair", p);
    BOOST_CHECK_EQUAL(p, (std::pair<int, std::string>(4, "foo")));
  }
}

template <typename Format>
static
void
option()
{
  std::stringstream stream;
  {
    boost::optional<int> empty;
    boost::optional<int> filled(42);
    typename Format::SerializerOut output(stream);
    output.serialize("empty", empty);
    output.serialize("filled", filled);
  }
  {
    boost::optional<int> empty;
    boost::optional<int> filled;
    typename Format::SerializerIn input(stream);
    input.serialize("empty", empty);
    input.serialize("filled", filled);
    BOOST_CHECK(!empty);
    BOOST_CHECK_EQUAL(filled.get(), 42);
  }
}

template <typename Format>
static
void
unique_ptr()
{
  std::stringstream stream;
  {
    std::unique_ptr<int> empty;
    std::unique_ptr<int> filled(new int(42));
    typename Format::SerializerOut output(stream);
    output.serialize("empty", empty);
    output.serialize("filled", filled);
  }
  {
    std::unique_ptr<int> empty;
    std::unique_ptr<int> filled;
    typename Format::SerializerIn input(stream);
    input.serialize("empty", empty);
    input.serialize("filled", filled);
    BOOST_CHECK(!empty);
    BOOST_CHECK_EQUAL(*filled, 42);
  }
}

template <typename Format>
static
void
unordered_map()
{
  std::unordered_map<int, std::string> map{
    {0, "zero"}, {1, "one"}, {2, "two"}};
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    output.serialize("map", map);
  }
  {
    std::unordered_map<int, std::string> res;
    typename Format::SerializerIn input(stream);
    input.serialize("map", res);
    BOOST_CHECK_EQUAL(map, res);
  }
}

template <typename Format>
static
void
buffer()
{
  elle::Buffer buffer("\x00\x01\x02\x03\x04", 5);
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    output.serialize("buffer", buffer);
  }
  {
    elle::Buffer res;
    typename Format::SerializerIn input(stream);
    input.serialize("buffer", res);
    BOOST_CHECK_EQUAL(buffer, res);
  }
}

class Super
  : public elle::serialization::VirtuallySerializable
{
public:
  Super(int i)
    : _i(i)
  {}

  Super(elle::serialization::SerializerIn& s)
  {
    this->serialize(s);
  }

  virtual
  void
  serialize(elle::serialization::Serializer& s)
  {
    s.serialize("super", this->_i);
  }

  virtual
  int
  type()
  {
    return this->_i;
  }

  ELLE_ATTRIBUTE_R(int, i);
};
static const elle::serialization::Hierarchy<Super>::Register<Super> _register_Super;

class Sub1
  : public Super
{
public:
  Sub1(int i)
    : Super(i - 1)
    , _i(i)
  {}

  Sub1(elle::serialization::SerializerIn& s)
    : Super(s)
    , _i(-1)
  {
    this->_serialize(s);
  }

  virtual
  void
  serialize(elle::serialization::Serializer& s) override
  {
    Super::serialize(s);
    this->_serialize(s);
  }

  void
  _serialize(elle::serialization::Serializer& s)
  {
    s.serialize("sub1", this->_i);
  }

  virtual
  int
  type() override
  {
    return this->_i;
  }

  ELLE_ATTRIBUTE_R(int, i);
};
static const elle::serialization::Hierarchy<Super>::Register<Sub1> _register_Sub1;

class Sub2
  : public Super
{
public:
  Sub2(int i)
    : Super(i - 1)
    , _i(i)
  {}

  Sub2(elle::serialization::SerializerIn& s)
    : Super(s)
    , _i(-1)
  {
    this->_serialize(s);
  }

  virtual
  void
  serialize(elle::serialization::Serializer& s) override
  {
    Super::serialize(s);
    this->_serialize(s);
  }

  void
  _serialize(elle::serialization::Serializer& s)
  {
    s.serialize("sub2", this->_i);
  }

  virtual
  int
  type() override
  {
    return this->_i;
  }

  ELLE_ATTRIBUTE_R(int, i);
};
static const elle::serialization::Hierarchy<Super>::Register<Sub2> _register_Sub2;

template <typename Format>
static
void
hierarchy()
{
  std::stringstream stream;
  {
    typename Format::SerializerOut output(stream);
    auto super = std::make_shared<Super>(0);
    auto s1 = std::make_shared<Sub1>(2);
    auto s2 = std::make_shared<Sub2>(3);
    output.serialize("super", super);
    output.serialize("sub1", s1);
    output.serialize("sub2", s2);
  }
  {
    typename Format::SerializerIn output(stream);
    std::shared_ptr<Super> ptr;
    output.serialize("super", ptr);
    BOOST_CHECK_EQUAL(ptr->type(), 0);
    output.serialize("sub1", ptr);
    BOOST_CHECK_EQUAL(ptr->type(), 2);
    output.serialize("sub2", ptr);
    BOOST_CHECK_EQUAL(ptr->type(), 3);
  }
}

class InPlace
{
public:
  InPlace(elle::serialization::SerializerIn&)
  {}

  InPlace(InPlace const&) = delete;

  void serialize(elle::serialization::Serializer&)
  {}
};

class OutPlace
{
public:
  OutPlace(OutPlace const&) = delete;

  void serialize(elle::serialization::Serializer&)
  {}
};

static
void
in_place()
{
  std::stringstream stream("{}");
  elle::serialization::json::SerializerIn input(stream);

  std::shared_ptr<InPlace> in;
  std::shared_ptr<InPlace> out;
  input.serialize("in", in);
  input.serialize("out", out);
}

static
void
json_type_error()
{
  std::stringstream stream(
    "{"
    "  \"int\": true"
    "}"
    );
  typename elle::serialization::json::SerializerIn input(stream);
  int v;
  try
  {
    input.serialize("int", v);
  }
  catch (elle::serialization::TypeError const& e)
  {
    BOOST_CHECK_EQUAL(e.field(), "int");
    BOOST_CHECK(*e.expected() == typeid(int64_t));
    BOOST_CHECK(*e.effective() == typeid(bool));
    return;
  }
  BOOST_FAIL("type error expected");
}

static
void
json_missing_key()
{
  std::stringstream stream(
    "{"
    "  \"a\": 0,"
    "  \"c\": 2"
    "}"
    );
  typename elle::serialization::json::SerializerIn input(stream);
  int v;
  try
  {
    input.serialize("b", v);
  }
  catch (elle::serialization::MissingKey const& e)
  {
    BOOST_CHECK_EQUAL(e.field(), "b");
    return;
  }
  BOOST_FAIL("type error expected");
}

ELLE_TEST_SUITE()
{
  auto& suite = boost::unit_test::framework::master_test_suite();
  suite.add(BOOST_TEST_CASE(fundamentals<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(object<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(object_composite<elle::serialization::Json>));
  auto list = &array<elle::serialization::Json, std::list>;
  suite.add(BOOST_TEST_CASE(list));
  auto deque = &array<elle::serialization::Json, std::deque>;
  suite.add(BOOST_TEST_CASE(deque));
  auto vector = &array<elle::serialization::Json, std::vector>;
  suite.add(BOOST_TEST_CASE(vector));
  suite.add(BOOST_TEST_CASE(pair<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(option<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(unique_ptr<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(unordered_map<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(buffer<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(hierarchy<elle::serialization::Json>));
  suite.add(BOOST_TEST_CASE(in_place));
  suite.add(BOOST_TEST_CASE(json_type_error));
  suite.add(BOOST_TEST_CASE(json_missing_key));
}
