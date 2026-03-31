#include "pg/catalog.hpp"

#include <format>

namespace getgresql::pg {

auto list_databases(const Connection& conn) -> Result<std::vector<DatabaseInfo>> {
    auto res = conn.exec(
        "SELECT d.datname, pg_catalog.pg_get_userbyid(d.datdba) AS owner, "
        "pg_catalog.pg_encoding_to_char(d.encoding) AS encoding, "
        "pg_catalog.pg_database_size(d.datname) AS size_bytes, "
        "pg_catalog.pg_size_pretty(pg_catalog.pg_database_size(d.datname)) AS size "
        "FROM pg_catalog.pg_database d "
        "WHERE d.datallowconn = true "
        "ORDER BY d.datname"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<DatabaseInfo> dbs;
    for (auto row : *res) {
        dbs.push_back({
            .name = std::string(row[0]),
            .owner = std::string(row[1]),
            .encoding = std::string(row[2]),
            .size = std::string(row[4]),
            .size_bytes = res->get_int(row.index(), 3).value_or(0),
        });
    }
    return dbs;
}

auto list_schemas(const Connection& conn, std::string_view /*database*/) -> Result<std::vector<SchemaInfo>> {
    auto res = conn.exec(
        "SELECT schema_name, schema_owner "
        "FROM information_schema.schemata "
        "WHERE schema_name NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
        "ORDER BY schema_name"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<SchemaInfo> schemas;
    for (auto row : *res) {
        schemas.push_back({
            .name = std::string(row[0]),
            .owner = std::string(row[1]),
        });
    }
    return schemas;
}

auto list_tables(const Connection& conn, std::string_view schema) -> Result<std::vector<TableInfo>> {
    auto sql = std::format(
        "SELECT c.relname, "
        "CASE c.relkind "
        "  WHEN 'r' THEN 'table' "
        "  WHEN 'v' THEN 'view' "
        "  WHEN 'm' THEN 'materialized view' "
        "  WHEN 'f' THEN 'foreign table' "
        "  WHEN 'p' THEN 'partitioned table' "
        "END AS type, "
        "c.reltuples::bigint AS row_estimate, "
        "pg_total_relation_size(c.oid) AS size_bytes, "
        "pg_size_pretty(pg_total_relation_size(c.oid)) AS size "
        "FROM pg_catalog.pg_class c "
        "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
        "WHERE n.nspname = '{}' "
        "AND c.relkind IN ('r', 'v', 'm', 'f', 'p') "
        "ORDER BY c.relname",
        schema  // safe: schema name from our own catalog queries
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<TableInfo> tables;
    for (auto row : *res) {
        tables.push_back({
            .schema = std::string(schema),
            .name = std::string(row[0]),
            .type = std::string(row[1]),
            .row_estimate = res->get_int(row.index(), 2).value_or(0),
            .size = std::string(row[4]),
            .size_bytes = res->get_int(row.index(), 3).value_or(0),
        });
    }
    return tables;
}

auto describe_columns(const Connection& conn, std::string_view schema,
                       std::string_view table) -> Result<std::vector<ColumnDetail>> {
    auto sql = std::format(
        "SELECT c.column_name, c.data_type, "
        "c.is_nullable = 'YES' AS nullable, "
        "COALESCE(c.column_default, '') AS default_value, "
        "CASE WHEN pk.column_name IS NOT NULL THEN true ELSE false END AS is_pk, "
        "c.ordinal_position "
        "FROM information_schema.columns c "
        "LEFT JOIN ("
        "  SELECT ku.column_name "
        "  FROM information_schema.table_constraints tc "
        "  JOIN information_schema.key_column_usage ku "
        "    ON tc.constraint_name = ku.constraint_name "
        "  WHERE tc.table_schema = '{}' AND tc.table_name = '{}' "
        "    AND tc.constraint_type = 'PRIMARY KEY'"
        ") pk ON pk.column_name = c.column_name "
        "WHERE c.table_schema = '{}' AND c.table_name = '{}' "
        "ORDER BY c.ordinal_position",
        schema, table, schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<ColumnDetail> cols;
    for (auto row : *res) {
        cols.push_back({
            .name = std::string(row[0]),
            .type = std::string(row[1]),
            .nullable = res->get_bool(row.index(), 2).value_or(true),
            .default_value = std::string(row[3]),
            .is_primary_key = res->get_bool(row.index(), 4).value_or(false),
            .ordinal = static_cast<int>(res->get_int(row.index(), 5).value_or(0)),
        });
    }
    return cols;
}

auto list_indexes(const Connection& conn, std::string_view schema,
                   std::string_view table) -> Result<std::vector<IndexInfo>> {
    auto sql = std::format(
        "SELECT i.relname AS index_name, "
        "pg_get_indexdef(i.oid) AS definition, "
        "ix.indisunique AS is_unique, "
        "ix.indisprimary AS is_primary, "
        "pg_size_pretty(pg_relation_size(i.oid)) AS size "
        "FROM pg_index ix "
        "JOIN pg_class i ON i.oid = ix.indexrelid "
        "JOIN pg_class t ON t.oid = ix.indrelid "
        "JOIN pg_namespace n ON n.oid = t.relnamespace "
        "WHERE n.nspname = '{}' AND t.relname = '{}' "
        "ORDER BY i.relname",
        schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<IndexInfo> indexes;
    for (auto row : *res) {
        indexes.push_back({
            .name = std::string(row[0]),
            .definition = std::string(row[1]),
            .is_unique = res->get_bool(row.index(), 2).value_or(false),
            .is_primary = res->get_bool(row.index(), 3).value_or(false),
            .size = std::string(row[4]),
        });
    }
    return indexes;
}

auto list_constraints(const Connection& conn, std::string_view schema,
                       std::string_view table) -> Result<std::vector<ConstraintInfo>> {
    auto sql = std::format(
        "SELECT con.conname, "
        "CASE con.contype "
        "  WHEN 'p' THEN 'PRIMARY KEY' "
        "  WHEN 'f' THEN 'FOREIGN KEY' "
        "  WHEN 'u' THEN 'UNIQUE' "
        "  WHEN 'c' THEN 'CHECK' "
        "  WHEN 'x' THEN 'EXCLUSION' "
        "END AS type, "
        "pg_get_constraintdef(con.oid) AS definition "
        "FROM pg_constraint con "
        "JOIN pg_class rel ON rel.oid = con.conrelid "
        "JOIN pg_namespace nsp ON nsp.oid = rel.relnamespace "
        "WHERE nsp.nspname = '{}' AND rel.relname = '{}' "
        "ORDER BY con.contype, con.conname",
        schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<ConstraintInfo> constraints;
    for (auto row : *res) {
        constraints.push_back({
            .name = std::string(row[0]),
            .type = std::string(row[1]),
            .definition = std::string(row[2]),
        });
    }
    return constraints;
}

auto list_foreign_keys(const Connection& conn, std::string_view schema,
                        std::string_view table) -> Result<std::vector<ForeignKeyInfo>> {
    auto sql = std::format(
        "SELECT con.conname, "
        "string_agg(a.attname, ', ' ORDER BY u.attposition) AS source_columns, "
        "c2.relname AS target_table, "
        "string_agg(a2.attname, ', ' ORDER BY u.attposition) AS target_columns "
        "FROM pg_constraint con "
        "JOIN pg_class c1 ON c1.oid = con.conrelid "
        "JOIN pg_namespace n ON n.oid = c1.relnamespace "
        "JOIN pg_class c2 ON c2.oid = con.confrelid "
        "JOIN LATERAL unnest(con.conkey, con.confkey) WITH ORDINALITY AS u(conkey, confkey, attposition) ON true "
        "JOIN pg_attribute a ON a.attnum = u.conkey AND a.attrelid = c1.oid "
        "JOIN pg_attribute a2 ON a2.attnum = u.confkey AND a2.attrelid = c2.oid "
        "WHERE con.contype = 'f' AND n.nspname = '{}' AND c1.relname = '{}' "
        "GROUP BY con.conname, c2.relname "
        "ORDER BY con.conname",
        schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<ForeignKeyInfo> fks;
    for (auto row : *res) {
        fks.push_back({
            .name = std::string(row[0]),
            .source_columns = std::string(row[1]),
            .target_table = std::string(row[2]),
            .target_columns = std::string(row[3]),
        });
    }
    return fks;
}

auto list_roles(const Connection& conn) -> Result<std::vector<RoleInfo>> {
    auto res = conn.exec(
        "SELECT r.rolname, r.rolsuper, r.rolcanlogin, r.rolcreatedb, r.rolcreaterole, "
        "r.rolconnlimit, COALESCE(r.rolvaliduntil::text, ''), "
        "(SELECT COALESCE(string_agg(r2.rolname, ', '), '') "
        " FROM pg_auth_members m JOIN pg_roles r2 ON r2.oid = m.roleid "
        " WHERE m.member = r.oid) "
        "FROM pg_roles r ORDER BY r.rolname"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<RoleInfo> roles;
    for (auto row : *res) {
        roles.push_back({
            .name = std::string(row[0]),
            .is_superuser = res->get_bool(row.index(), 1).value_or(false),
            .can_login = res->get_bool(row.index(), 2).value_or(false),
            .can_create_db = res->get_bool(row.index(), 3).value_or(false),
            .can_create_role = res->get_bool(row.index(), 4).value_or(false),
            .connection_limit = static_cast<int>(res->get_int(row.index(), 5).value_or(-1)),
            .valid_until = std::string(row[6]),
            .member_of = std::string(row[7]),
        });
    }
    return roles;
}

auto list_extensions(const Connection& conn) -> Result<std::vector<ExtensionInfo>> {
    auto res = conn.exec(
        "SELECT e.extname, e.extversion, n.nspname, COALESCE(c.description, '') "
        "FROM pg_extension e "
        "JOIN pg_namespace n ON n.oid = e.extnamespace "
        "LEFT JOIN pg_description c ON c.objoid = e.oid "
        "  AND c.classoid = 'pg_extension'::regclass "
        "ORDER BY e.extname"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<ExtensionInfo> extensions;
    for (auto row : *res) {
        extensions.push_back({
            .name = std::string(row[0]),
            .version = std::string(row[1]),
            .schema = std::string(row[2]),
            .description = std::string(row[3]),
        });
    }
    return extensions;
}

auto list_functions(const Connection& conn, std::string_view schema) -> Result<std::vector<FunctionInfo>> {
    auto sql = std::format(
        "SELECT p.proname, pg_get_function_arguments(p.oid), "
        "pg_get_function_result(p.oid), l.lanname, "
        "CASE p.provolatile "
        "  WHEN 'v' THEN 'VOLATILE' "
        "  WHEN 's' THEN 'STABLE' "
        "  WHEN 'i' THEN 'IMMUTABLE' "
        "END, "
        "p.prorettype = 'trigger'::regtype "
        "FROM pg_proc p "
        "JOIN pg_namespace n ON n.oid = p.pronamespace "
        "JOIN pg_language l ON l.oid = p.prolang "
        "WHERE n.nspname = '{}' AND p.prokind IN ('f', 'p') "
        "ORDER BY p.proname",
        schema
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<FunctionInfo> functions;
    for (auto row : *res) {
        functions.push_back({
            .name = std::string(row[0]),
            .arguments = std::string(row[1]),
            .return_type = std::string(row[2]),
            .language = std::string(row[3]),
            .volatility = std::string(row[4]),
            .is_trigger = res->get_bool(row.index(), 5).value_or(false),
        });
    }
    return functions;
}

auto list_sequences(const Connection& conn, std::string_view schema) -> Result<std::vector<SequenceInfo>> {
    auto sql = std::format(
        "SELECT sequencename, data_type, "
        "COALESCE(last_value, start_value), start_value, "
        "increment_by, min_value, max_value, cycle "
        "FROM pg_sequences "
        "WHERE schemaname = '{}' "
        "ORDER BY sequencename",
        schema
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<SequenceInfo> sequences;
    for (auto row : *res) {
        sequences.push_back({
            .name = std::string(row[0]),
            .data_type = std::string(row[1]),
            .current_value = res->get_int(row.index(), 2).value_or(0),
            .start_value = res->get_int(row.index(), 3).value_or(0),
            .increment = res->get_int(row.index(), 4).value_or(1),
            .min_value = res->get_int(row.index(), 5).value_or(0),
            .max_value = res->get_int(row.index(), 6).value_or(0),
            .cycle = res->get_bool(row.index(), 7).value_or(false),
        });
    }
    return sequences;
}

auto index_usage_stats(const Connection& conn, std::string_view schema) -> Result<std::vector<IndexUsageInfo>> {
    auto sql = std::format(
        "SELECT s.schemaname, s.relname, s.indexrelname, "
        "pg_relation_size(s.indexrelid), "
        "pg_size_pretty(pg_relation_size(s.indexrelid)), "
        "COALESCE(s.idx_scan, 0), "
        "COALESCE(s.idx_tup_read, 0), "
        "COALESCE(s.idx_tup_fetch, 0) "
        "FROM pg_stat_user_indexes s "
        "WHERE s.schemaname = '{}' "
        "ORDER BY s.idx_scan ASC, pg_relation_size(s.indexrelid) DESC",
        schema
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<IndexUsageInfo> stats;
    for (auto row : *res) {
        stats.push_back({
            .schema = std::string(row[0]),
            .table = std::string(row[1]),
            .index_name = std::string(row[2]),
            .index_size_bytes = res->get_int(row.index(), 3).value_or(0),
            .index_size = std::string(row[4]),
            .idx_scan = res->get_int(row.index(), 5).value_or(0),
            .idx_tup_read = res->get_int(row.index(), 6).value_or(0),
            .idx_tup_fetch = res->get_int(row.index(), 7).value_or(0),
        });
    }
    return stats;
}

auto database_size_breakdown(const Connection& conn) -> Result<std::vector<SizeBreakdown>> {
    auto res = conn.exec(
        "SELECT n.nspname, "
        "SUM(pg_total_relation_size(c.oid))::bigint, "
        "pg_size_pretty(SUM(pg_total_relation_size(c.oid))::bigint), "
        "COUNT(*) FILTER (WHERE c.relkind IN ('r','p')), "
        "COUNT(*) FILTER (WHERE c.relkind = 'i') "
        "FROM pg_class c "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "WHERE n.nspname NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
        "AND c.relkind IN ('r', 'i', 'p', 'm') "
        "GROUP BY n.nspname "
        "ORDER BY SUM(pg_total_relation_size(c.oid)) DESC"
    );
    if (!res) return std::unexpected(res.error());

    std::vector<SizeBreakdown> breakdown;
    for (auto row : *res) {
        breakdown.push_back({
            .schema = std::string(row[0]),
            .size_bytes = res->get_int(row.index(), 1).value_or(0),
            .size = std::string(row[2]),
            .table_count = static_cast<int>(res->get_int(row.index(), 3).value_or(0)),
            .index_count = static_cast<int>(res->get_int(row.index(), 4).value_or(0)),
        });
    }
    return breakdown;
}

auto get_function_source(const Connection& conn, std::string_view schema,
                          std::string_view name, std::string_view /*args*/) -> Result<FunctionSource> {
    auto sql = std::format(
        "SELECT p.proname, l.lanname, p.prosrc, pg_get_functiondef(p.oid) "
        "FROM pg_proc p "
        "JOIN pg_namespace n ON n.oid = p.pronamespace "
        "JOIN pg_language l ON l.oid = p.prolang "
        "WHERE n.nspname = '{}' AND p.proname = '{}' "
        "LIMIT 1",
        schema, name
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    if (res->row_count() == 0) {
        return make_error(PgQueryError{
            .message = std::format("function {}.{} not found", schema, name),
            .sql = sql,
        });
    }

    auto row = (*res).begin();
    return FunctionSource{
        .name = std::string((*row)[0]),
        .language = std::string((*row)[1]),
        .source = std::string((*row)[2]),
        .definition = std::string((*row)[3]),
    };
}

auto table_row_count(const Connection& conn, std::string_view schema,
                      std::string_view table) -> Result<long long> {
    auto sql = std::format(
        "SELECT COUNT(*) FROM \"{}\".\"{}\"",
        schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    return res->get_int(0, 0).value_or(0);
}

auto preview_rows(const Connection& conn, std::string_view schema,
                   std::string_view table, int limit) -> Result<PgResult> {
    auto sql = std::format(
        "SELECT * FROM \"{}\".\"{}\" LIMIT {}",
        schema, table, limit
    );

    return conn.exec(sql);
}

// ─── DDL Generation ─────────────────────────────────────────────────

auto table_ddl(const Connection& conn, std::string_view schema, std::string_view table) -> Result<std::string> {
    std::string ddl;

    // Column definitions
    auto col_sql = std::format(
        "SELECT column_name, data_type, character_maximum_length, "
        "is_nullable, column_default, "
        "CASE WHEN udt_name LIKE 'int%' THEN udt_name "
        "     WHEN data_type = 'character varying' THEN 'varchar(' || character_maximum_length || ')' "
        "     WHEN data_type = 'character' THEN 'char(' || character_maximum_length || ')' "
        "     WHEN data_type = 'numeric' THEN 'numeric(' || COALESCE(numeric_precision::text,'') || ',' || COALESCE(numeric_scale::text,'') || ')' "
        "     WHEN data_type = 'ARRAY' THEN udt_name "
        "     WHEN data_type = 'USER-DEFINED' THEN udt_name "
        "     ELSE data_type END AS col_type "
        "FROM information_schema.columns "
        "WHERE table_schema = '{}' AND table_name = '{}' "
        "ORDER BY ordinal_position",
        schema, table
    );
    auto cols = conn.exec(col_sql);
    if (!cols) return std::unexpected(cols.error());

    ddl += std::format("CREATE TABLE \"{}\".\"{}\" (\n", schema, table);
    for (int i = 0; auto row : *cols) {
        if (i > 0) ddl += ",\n";
        ddl += std::format("    \"{}\" {}", std::string(row[0]), std::string(row[5]));
        if (std::string(row[3]) == "NO") ddl += " NOT NULL";
        auto def = std::string(row[4]);
        if (!def.empty()) ddl += std::format(" DEFAULT {}", def);
        ++i;
    }

    // Primary key
    auto pk_sql = std::format(
        "SELECT string_agg(a.attname, ', ' ORDER BY array_position(i.indkey, a.attnum)) "
        "FROM pg_index i "
        "JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey) "
        "WHERE i.indrelid = '\"{}\".\"{}\"\t'::regclass AND i.indisprimary",
        schema, table
    );
    auto pk = conn.exec(pk_sql);
    if (pk && pk->row_count() > 0 && !pk->is_null(0, 0)) {
        ddl += std::format(",\n    PRIMARY KEY ({})", pk->get(0, 0));
    }

    // Unique constraints
    auto uq_sql = std::format(
        "SELECT conname, pg_get_constraintdef(oid) "
        "FROM pg_constraint "
        "WHERE conrelid = '\"{}\".\"{}\"\t'::regclass AND contype = 'u'",
        schema, table
    );
    auto uqs = conn.exec(uq_sql);
    if (uqs) {
        for (auto row : *uqs) {
            ddl += std::format(",\n    CONSTRAINT \"{}\" {}", std::string(row[0]), std::string(row[1]));
        }
    }

    // Foreign keys
    auto fk_sql = std::format(
        "SELECT conname, pg_get_constraintdef(oid) "
        "FROM pg_constraint "
        "WHERE conrelid = '\"{}\".\"{}\"\t'::regclass AND contype = 'f'",
        schema, table
    );
    auto fks = conn.exec(fk_sql);
    if (fks) {
        for (auto row : *fks) {
            ddl += std::format(",\n    CONSTRAINT \"{}\" {}", std::string(row[0]), std::string(row[1]));
        }
    }

    // Check constraints
    auto ck_sql = std::format(
        "SELECT conname, pg_get_constraintdef(oid) "
        "FROM pg_constraint "
        "WHERE conrelid = '\"{}\".\"{}\"\t'::regclass AND contype = 'c'",
        schema, table
    );
    auto cks = conn.exec(ck_sql);
    if (cks) {
        for (auto row : *cks) {
            ddl += std::format(",\n    CONSTRAINT \"{}\" {}", std::string(row[0]), std::string(row[1]));
        }
    }

    ddl += "\n);\n";

    // Indexes (non-primary, non-unique-constraint)
    auto idx_sql = std::format(
        "SELECT indexdef FROM pg_indexes "
        "WHERE schemaname = '{}' AND tablename = '{}' "
        "AND indexname NOT IN ("
        "  SELECT conname FROM pg_constraint "
        "  WHERE conrelid = '\"{}\".\"{}\"\t'::regclass AND contype IN ('p','u')"
        ")",
        schema, table, schema, table
    );
    auto idxs = conn.exec(idx_sql);
    if (idxs) {
        for (auto row : *idxs) {
            ddl += std::format("\n{};\n", std::string(row[0]));
        }
    }

    // Table and column comments
    auto comment_sql = std::format(
        "SELECT obj_description(c.oid) FROM pg_class c "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "WHERE n.nspname = '{}' AND c.relname = '{}'",
        schema, table
    );
    auto tcmt = conn.exec(comment_sql);
    if (tcmt && tcmt->row_count() > 0 && !tcmt->is_null(0, 0)) {
        auto cmt = std::string(tcmt->get(0, 0));
        if (!cmt.empty()) {
            // Escape single quotes in comment
            std::string escaped;
            for (char c : cmt) {
                if (c == '\'') escaped += "''";
                else escaped += c;
            }
            ddl += std::format("\nCOMMENT ON TABLE \"{}\".\"{}\" IS '{}';\n", schema, table, escaped);
        }
    }

    return ddl;
}

// ─── Column Statistics ──────────────────────────────────────────────

auto column_statistics(const Connection& conn, std::string_view schema,
                        std::string_view table) -> Result<std::vector<ColumnStats>> {
    auto sql = std::format(
        "SELECT s.attname, "
        "COALESCE(c.data_type, 'unknown') AS data_type, "
        "s.null_frac, s.n_distinct, s.avg_width, "
        "s.most_common_vals::text, s.most_common_freqs::text, "
        "s.histogram_bounds::text, s.correlation "
        "FROM pg_stats s "
        "LEFT JOIN information_schema.columns c "
        "  ON c.table_schema = s.schemaname AND c.table_name = s.tablename "
        "  AND c.column_name = s.attname "
        "WHERE s.schemaname = '{}' AND s.tablename = '{}' "
        "ORDER BY s.attname",
        schema, table
    );

    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());

    std::vector<ColumnStats> stats;
    for (auto row : *res) {
        stats.push_back({
            .column_name = std::string(row[0]),
            .data_type = std::string(row[1]),
            .null_fraction = res->get_double(row.index(), 2).value_or(0.0),
            .n_distinct = res->get_int(row.index(), 3).value_or(0),
            .avg_width = static_cast<int>(res->get_int(row.index(), 4).value_or(0)),
            .most_common_vals = row.is_null(5) ? std::string() : std::string(row[5]),
            .most_common_freqs = row.is_null(6) ? std::string() : std::string(row[6]),
            .histogram_bounds = row.is_null(7) ? std::string() : std::string(row[7]),
            .correlation = res->get_double(row.index(), 8).value_or(0.0),
        });
    }
    return stats;
}

// ─── Schema ERD ─────────────────────────────────────────────────────

auto schema_erd(const Connection& conn, std::string_view schema) -> Result<ERDData> {
    ERDData erd;

    // Tables with columns
    auto tbl_sql = std::format(
        "SELECT c.table_name, "
        "CASE WHEN t.table_type = 'VIEW' THEN 'view' ELSE 'table' END, "
        "c.column_name, "
        "CASE WHEN c.data_type = 'USER-DEFINED' THEN c.udt_name "
        "     WHEN c.data_type = 'character varying' THEN 'varchar' "
        "     WHEN c.data_type = 'ARRAY' THEN c.udt_name "
        "     ELSE c.data_type END "
        "FROM information_schema.columns c "
        "JOIN information_schema.tables t ON t.table_schema = c.table_schema AND t.table_name = c.table_name "
        "WHERE c.table_schema = '{}' "
        "ORDER BY c.table_name, c.ordinal_position",
        schema
    );
    auto tbls = conn.exec(tbl_sql);
    if (!tbls) return std::unexpected(tbls.error());

    std::string current_table;
    for (auto row : *tbls) {
        auto tname = std::string(row[0]);
        if (tname != current_table) {
            erd.tables.push_back({.name = tname, .type = std::string(row[1]), .columns = {}});
            current_table = tname;
        }
        erd.tables.back().columns.emplace_back(std::string(row[2]), std::string(row[3]));
    }

    // Foreign key relationships
    auto fk_sql = std::format(
        "SELECT tc.constraint_name, tc.table_name AS source, "
        "string_agg(DISTINCT kcu.column_name, ', ') AS source_cols, "
        "ccu.table_name AS target, "
        "string_agg(DISTINCT ccu.column_name, ', ') AS target_cols "
        "FROM information_schema.table_constraints tc "
        "JOIN information_schema.key_column_usage kcu "
        "  ON kcu.constraint_name = tc.constraint_name AND kcu.table_schema = tc.table_schema "
        "JOIN information_schema.constraint_column_usage ccu "
        "  ON ccu.constraint_name = tc.constraint_name AND ccu.table_schema = tc.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_schema = '{}' "
        "GROUP BY tc.constraint_name, tc.table_name, ccu.table_name",
        schema
    );
    auto fks = conn.exec(fk_sql);
    if (!fks) return std::unexpected(fks.error());

    for (auto row : *fks) {
        erd.relationships.push_back({
            .constraint_name = std::string(row[0]),
            .source_table = std::string(row[1]),
            .source_columns = std::string(row[2]),
            .target_table = std::string(row[3]),
            .target_columns = std::string(row[4]),
        });
    }

    return erd;
}

// ─── Quick Actions ──────────────────────────────────────────────────

auto vacuum_table(const Connection& conn, std::string_view schema,
                   std::string_view table) -> Result<bool> {
    auto sql = std::format("VACUUM \"{}\".\"{}\"\t", schema, table);
    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());
    return true;
}

auto analyze_table(const Connection& conn, std::string_view schema,
                    std::string_view table) -> Result<bool> {
    auto sql = std::format("ANALYZE \"{}\".\"{}\"\t", schema, table);
    auto res = conn.exec(sql);
    if (!res) return std::unexpected(res.error());
    return true;
}

auto completion_metadata(const Connection& conn) -> Result<CompletionData> {
    // Get schemas
    auto schema_res = conn.exec(
        "SELECT nspname FROM pg_catalog.pg_namespace "
        "WHERE nspname NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
        "AND nspname NOT LIKE 'pg_temp_%' AND nspname NOT LIKE 'pg_toast_temp_%' "
        "ORDER BY nspname"
    );
    if (!schema_res) return std::unexpected(schema_res.error());

    CompletionData data;
    for (auto row : *schema_res) {
        data.schemas.push_back(std::string(row[0]));
    }

    // Get all tables with columns in one join query
    auto col_res = conn.exec(
        "SELECT c.table_schema, c.table_name, "
        "CASE WHEN t.table_type = 'VIEW' THEN 'view' ELSE 'table' END AS ttype, "
        "c.column_name, c.data_type "
        "FROM information_schema.columns c "
        "JOIN information_schema.tables t "
        "  ON t.table_schema = c.table_schema AND t.table_name = c.table_name "
        "WHERE c.table_schema NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
        "ORDER BY c.table_schema, c.table_name, c.ordinal_position"
    );
    if (!col_res) return std::unexpected(col_res.error());

    std::string current_schema, current_table;
    for (auto row : *col_res) {
        auto schema = std::string(row[0]);
        auto table = std::string(row[1]);
        auto ttype = std::string(row[2]);
        auto col_name = std::string(row[3]);
        auto col_type = std::string(row[4]);

        if (schema != current_schema || table != current_table) {
            data.tables.push_back({
                .schema = schema,
                .name = table,
                .type = ttype,
                .columns = {},
            });
            current_schema = schema;
            current_table = table;
        }
        data.tables.back().columns.push_back({
            .name = col_name,
            .type = col_type,
        });
    }

    return data;
}

} // namespace getgresql::pg
