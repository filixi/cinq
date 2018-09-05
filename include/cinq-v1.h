// Unit test for functionalities
// unit test of each single query should cover the following cases:
// For data source $:
//  - is empty
//  - has one element
//  - has at least five elements
//
// For Result #:
//  if the result is a enumerable
//  - is empty (if possible, data source must not be empty)
//  - has one element
//  - has multiple elements (if possible)
//  - includes the first element of data source (if possible)
//  - excludes the first element of data source (if possible)
//  - includes the last element of data source, size(data source) > 1 (if possible)
//  - excludes the last element of data source, size(data source) > 1 (if possible)
//  - includes an element in middle, size(data source) > 1 (if possible)
//  - excludes an element in middle, size(data source) > 1 (if possible)
//  - is of size greater than one and includes all elements of all data source(s) (if possible)
//  if the result is a single arithmetic type which doesn't come from the element(s) of data source(s)
//  - equals to zero (if possible)
//  - equals to one due to the first element of data source which contains one and multiple element(s) (if possible)
//  - equals to one due to the last element of data source which contains one and multiple element(s) (if possible)
//  - equals to one due to an element in middle of data source
//  - is non-zero but is less than the size of data source (if possible)
//  - equals to the size of data source (if possible)

// Supplementary rules :
//  - test case can be combined.
//  - test can use arbitary element type. Case of using user-defined types is tested by semantics test.
//  - if the query can take infinit data sources, test up to the minimum count of data source + 1.
//      And an additional trivial test for at least minimum count + 2.
//  - if the query takes multiple data sources, the input should consist of the full permutation of full combination of the case for data source if possible.
//  - if the query takes multiple data sources, the output should be tested for applying on every single data source.
//  - chained query is tested by chained query generator, and the result is compared with the result of
//      step by step query (insert ToVector in between). The correctness of this test is base on the assumption that each individual query is correct.
