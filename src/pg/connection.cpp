#include "pg/connection.hpp"

// Explicit template instantiations to reduce compile times.
// The heavy template machinery is compiled once here rather than
// in every translation unit that includes connection.hpp.

namespace getgresql::pg {

template class PgConnection<Disconnected>;
template class PgConnection<Connected>;
template class PgConnection<InTransaction>;

} // namespace getgresql::pg
