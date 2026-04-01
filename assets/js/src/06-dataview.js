// getgreSQL — DataView: Virtual Grid Engine
// High-performance data grid with virtual scrolling, multi-cell selection,
// instant filtering, sorting, copy/paste, result diff, keyboard navigation.
//
// One DataGrid instance per .data-view container. Stores all row data in
// memory, renders only visible rows for 60fps scrolling at any dataset size.

(function() {
'use strict';

var ROW_H = 32;
var BUFFER = 20;
var VIRT_THRESHOLD = 200;
var activeGrid = null;
var _prevSnapshot = null;

// ─── Helpers ────────────────────────────────────────────────────────

function esc(s) {
    if (!s) return '';
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

function encP(s) { return encodeURIComponent(s || ''); }

function detectColType(rows, ci) {
    var s = 0, num = 0, dt = 0, js = 0, n = Math.min(rows.length, 30);
    for (var i = 0; i < n; i++) {
        var v = rows[i].full[ci];
        if (!v || rows[i].nulls[ci]) continue;
        s++;
        if (!isNaN(parseFloat(v)) && isFinite(v)) num++;
        if (/^\d{4}-\d{2}-\d{2}/.test(v)) dt++;
        if (/^\s*[\[{]/.test(v)) js++;
    }
    if (!s) return '';
    if (num / s > 0.8) return 'numeric';
    if (dt / s > 0.8) return 'date';
    if (js / s > 0.5) return 'json';
    return '';
}

// ─── DataGrid ───────────────────────────────────────────────────────

function DataGrid(el) {
    this.el = el;
    this.table = el.querySelector('.dv-table');
    this.wrapper = el.querySelector('.table-wrapper.scrollable');
    if (!this.table || !this.wrapper) return;

    this.editable = el.getAttribute('data-editable') === 'true';
    this.db = el.getAttribute('data-db') || '';
    this.schema = el.getAttribute('data-schema') || '';
    this.tableName = el.getAttribute('data-table') || '';

    this.cols = [];
    this.hasRowNum = false;
    this.hasActions = false;
    this.dataStart = 0;

    this.rows = [];
    this.view = null;
    this.viewMap = {};

    this.sortCol = -1;
    this.sortDir = '';
    this.globalQ = '';
    this.colFilters = {};

    this.anchor = null;   // {vr, c} view-space
    this.cursor = null;   // {vr, c} view-space
    this.selRows = new Set();
    this.lastClickRow = -1;

    this.virtual = false;
    this.vStart = -1;
    this.vEnd = -1;
    this.scrollRaf = 0;
    this.lastScrollTop = -1;

    this.prevData = _prevSnapshot;
    this.diffActive = false;
    this.diffMap = null;

    this.inf = { loading: false, done: false, page: 0, limit: 0, baseUrl: '' };

    this._init();
}

var P = DataGrid.prototype;

P._init = function() {
    this._extractCols();
    this._extractData();
    _prevSnapshot = this._snapshot();
    this._buildView();
    this._enhanceToolbar();
    this._addColFilters();
    this._autoSize();
    this._initResize();

    if (this.rows.length >= VIRT_THRESHOLD) {
        this._enableVirtual();
    }

    this._setupInfScroll();
    this._updateCount();

    var self = this;
    this.table.addEventListener('htmx:afterSettle', function(e) {
        self._syncHtmx(e);
    });

    // Reposition explain button when table scrolls
    this.wrapper.addEventListener('scroll', function() {
        if (self.cursor && window._dvExplainBtn && window._dvExplainBtn.style.display !== 'none') {
            self._posExplain();
        }
    }, { passive: true });
};

// ─── Column Extraction ──────────────────────────────────────────────

P._extractCols = function() {
    var thead = this.table.tHead;
    if (!thead || !thead.rows[0]) return;
    var ths = thead.rows[0].cells;
    this.hasRowNum = !!this.table.querySelector('.row-num-header');
    this.hasActions = !!this.table.querySelector('.dv-actions-header');
    this.dataStart = this.hasRowNum ? 1 : 0;

    for (var i = 0; i < ths.length; i++) {
        var th = ths[i];
        if (th.classList.contains('row-num-header') || th.classList.contains('dv-actions-header')) continue;
        var tx = th.querySelector('.dv-th-text');
        this.cols.push({
            name: tx ? tx.textContent.trim() : th.textContent.trim(),
            type: th.getAttribute('data-type') || '',
            sortable: th.classList.contains('sortable'),
            tableOid: '0',
            el: th
        });
    }
};

// ─── Data Extraction ────────────────────────────────────────────────

P._extractData = function() {
    var tbody = this.table.tBodies[0];
    if (!tbody) return;
    var trs = tbody.rows;
    var endOff = this.hasActions ? 1 : 0;

    for (var i = 0; i < trs.length; i++) {
        var tr = trs[i];
        if (tr.classList.contains('dv-spacer') || tr.classList.contains('dv-filter-row')) continue;
        var ctid = tr.getAttribute('data-ctid') || '';
        var cells = [], nulls = [], full = [];

        for (var j = this.dataStart; j < tr.cells.length - endOff; j++) {
            var td = tr.cells[j];
            var span = td.querySelector('.editable-cell, .dv-cell, .null-value');
            var isN = td.querySelector('.null-value') !== null;
            var v, fv;
            if (span) {
                fv = span.getAttribute('data-full') || span.textContent.trim();
                v = span.textContent.trim();
                if (i === 0 && span.getAttribute('data-table-oid')) {
                    var ci = j - this.dataStart;
                    if (ci < this.cols.length) this.cols[ci].tableOid = span.getAttribute('data-table-oid');
                }
            } else {
                v = td.textContent.trim();
                fv = v;
            }
            cells.push(v);
            nulls.push(isN);
            full.push(fv);
        }

        tr.setAttribute('data-di', this.rows.length);
        this.rows.push({ ctid: ctid, cells: cells, nulls: nulls, full: full, _tr: tr });
    }

    for (var c = 0; c < this.cols.length; c++) {
        if (!this.cols[c].type) this.cols[c].type = detectColType(this.rows, c);
    }
};

// ─── View Layer (filter + sort → index array) ───────────────────────

P._buildView = function() {
    var needsView = this.globalQ || Object.keys(this.colFilters).length > 0 || this.sortCol >= 0;
    if (!needsView) {
        this.view = null;
        this.viewMap = {};
        return;
    }

    var idx = [];
    for (var i = 0; i < this.rows.length; i++) idx.push(i);

    var gq = this.globalQ;
    var cf = this.colFilters;
    var self = this;

    if (gq) {
        idx = idx.filter(function(i) {
            var r = self.rows[i];
            for (var c = 0; c < r.full.length; c++) {
                if (r.full[c] && r.full[c].toLowerCase().indexOf(gq) !== -1) return true;
            }
            return false;
        });
    }

    var cfk = Object.keys(cf);
    for (var k = 0; k < cfk.length; k++) {
        var ci = parseInt(cfk[k]);
        var q = cf[cfk[k]];
        if (!q) continue;
        idx = idx.filter(function(i) {
            var v = self.rows[i].full[ci];
            return v && v.toLowerCase().indexOf(q) !== -1;
        });
    }

    if (this.sortCol >= 0) {
        var sc = this.sortCol, sd = this.sortDir;
        var isNum = this.cols[sc] && (this.cols[sc].type === 'numeric' || detectColType(this.rows, sc) === 'numeric');
        idx.sort(function(a, b) {
            var av = self.rows[a].full[sc] || '';
            var bv = self.rows[b].full[sc] || '';
            var an = self.rows[a].nulls[sc];
            var bn = self.rows[b].nulls[sc];
            if (an && !bn) return 1;
            if (!an && bn) return -1;
            if (an && bn) return 0;
            var cmp;
            if (isNum) cmp = (parseFloat(av) || 0) - (parseFloat(bv) || 0);
            else cmp = av.localeCompare(bv, undefined, { sensitivity: 'base' });
            return sd === 'asc' ? cmp : -cmp;
        });
    }

    this.view = idx;
    this.viewMap = {};
    for (var i = 0; i < idx.length; i++) this.viewMap[idx[i]] = i;
};

P.viewLen = function() { return this.view ? this.view.length : this.rows.length; };
P.dataIdx = function(vi) { return this.view ? this.view[vi] : vi; };
P.viewIdx = function(di) {
    if (!this.view) return di < this.rows.length ? di : -1;
    return this.viewMap.hasOwnProperty(di) ? this.viewMap[di] : -1;
};

// ─── Virtual Scroll ─────────────────────────────────────────────────

P._enableVirtual = function() {
    this.virtual = true;
    for (var i = 0; i < this.rows.length; i++) this.rows[i]._tr = null;
    var self = this;
    this.wrapper.addEventListener('scroll', function() { self._onScroll(); }, { passive: true });
    this._renderVis();
};

P._onScroll = function() {
    if (window._dvEditing) return;
    var st = this.wrapper.scrollTop;
    if (st === this.lastScrollTop) return;
    this.lastScrollTop = st;
    var self = this;
    if (this.scrollRaf) return;
    this.scrollRaf = requestAnimationFrame(function() {
        self.scrollRaf = 0;
        self._renderVis();
    });
};

P._renderVis = function() {
    if (!this.virtual) return;
    var st = this.wrapper.scrollTop;
    var vh = this.wrapper.clientHeight;
    var total = this.viewLen();

    var start = Math.max(0, Math.floor(st / ROW_H) - BUFFER);
    var end = Math.min(total, Math.ceil((st + vh) / ROW_H) + BUFFER);

    if (start === this.vStart && end === this.vEnd) return;
    this.vStart = start;
    this.vEnd = end;

    var cs = (this.hasRowNum ? 1 : 0) + this.cols.length + (this.hasActions ? 1 : 0);
    var topH = start * ROW_H;
    var botH = Math.max(0, (total - end) * ROW_H);

    var p = ['<tr class="dv-spacer"><td colspan="', cs, '" style="height:', topH, 'px;padding:0;border:none"></td></tr>'];
    for (var i = start; i < end; i++) p.push(this._rowHTML(this.dataIdx(i), i));
    p.push('<tr class="dv-spacer"><td colspan="', cs, '" style="height:', botH, 'px;padding:0;border:none"></td></tr>');

    this.table.tBodies[0].innerHTML = p.join('');
    this._applySel();
    if (window.htmx) htmx.process(this.table.tBodies[0]);
};

// ─── Non-Virtual Re-render ──────────────────────────────────────────

P._renderNV = function() {
    if (this.virtual) return;
    var tbody = this.table.tBodies[0];
    if (!tbody) return;

    if (!this.view) {
        for (var i = 0; i < this.rows.length; i++) {
            var tr = this.rows[i]._tr;
            if (tr) { tr.style.display = ''; tbody.appendChild(tr); }
        }
    } else {
        for (var i = 0; i < this.rows.length; i++) {
            if (this.rows[i]._tr) this.rows[i]._tr.style.display = 'none';
        }
        for (var i = 0; i < this.view.length; i++) {
            var tr = this.rows[this.view[i]]._tr;
            if (tr) { tr.style.display = ''; tbody.appendChild(tr); }
        }
    }
    this._applySel();
};

// ─── Row HTML (virtual mode) ────────────────────────────────────────

P._rowHTML = function(di, vi) {
    var row = this.rows[di];
    var p = ['<tr data-di="', di, '"'];
    if (row.ctid) p.push(' data-ctid="', esc(row.ctid), '"');

    var cls = [];
    if (this.selRows.has(di)) cls.push('dv-row-selected');
    if (this.cursor && this.dataIdx(this.cursor.vr) === di) cls.push('row-active');
    if (this.diffActive && this.diffMap && this.diffMap[di]) {
        var dt = this.diffMap[di].type;
        if (dt !== 'same') cls.push('diff-row-' + dt);
    }
    if (cls.length) p.push(' class="', cls.join(' '), '"');
    p.push('>');

    if (this.hasRowNum) p.push('<td class="row-num">', vi + 1, '</td>');

    for (var c = 0; c < this.cols.length; c++) {
        var val = c < row.cells.length ? row.cells[c] : '';
        var isN = c < row.nulls.length ? row.nulls[c] : false;
        var fv = c < row.full.length ? row.full[c] : val;
        var col = this.cols[c];
        var canEdit = row.ctid && (this.editable || (col.tableOid && col.tableOid !== '0'));

        var tdCls = '';
        if (col.type === 'numeric' && !isN) tdCls = ' class="dv-num"';
        if (this.diffActive && this.diffMap && this.diffMap[di] && this.diffMap[di].changedCols.has(c)) tdCls = ' class="diff-cell-changed"';

        p.push('<td', tdCls, '>');
        if (canEdit) p.push(this._cellEdit(val, isN, fv, col, row.ctid));
        else p.push(this._cellRO(val, isN, fv, col));
        p.push('</td>');
    }

    if (this.hasActions && this.editable) {
        p.push('<td class="dv-actions"><button class="btn btn-sm btn-danger" ',
            'hx-post="/db/', esc(this.db), '/schema/', esc(this.schema), '/table/', esc(this.tableName),
            '/delete-row" hx-vals=\'{"ctid":"', esc(row.ctid),
            '"}\' hx-target="#tab-content" hx-swap="innerHTML" hx-confirm="Delete this row?">&#10005;</button></td>');
    }

    p.push('</tr>');
    return p.join('');
};

P._cellEdit = function(val, isN, fv, col, ctid) {
    var p = ['<span class="'];
    if (isN) p.push('null-value editable-cell');
    else { p.push('editable-cell'); if (fv.length > 80) p.push(' dv-cell-long'); }
    p.push('" data-col="', esc(col.name), '"');
    if (col.tableOid) p.push(' data-table-oid="', col.tableOid, '"');
    if (!isN && fv.length > 80) p.push(' data-full="', esc(fv), '"');
    if (ctid) p.push(' data-ctid="', esc(ctid), '"');

    p.push(' hx-get="/dv/edit-cell?db=', encP(this.db),
        '&amp;schema=', encP(this.schema),
        '&amp;table=', encP(this.tableName),
        '&amp;table_oid=', col.tableOid || '0',
        '&amp;col=', encP(col.name),
        '&amp;ctid=', encP(ctid),
        '&amp;val=', encP(isN ? '' : fv),
        '" hx-trigger="dblclick" hx-target="closest td" hx-swap="innerHTML">');

    if (isN) p.push('NULL</span>');
    else if (fv.length > 80) p.push(esc(fv.substring(0, 60)), '&hellip;</span>');
    else p.push(esc(val), '</span>');
    return p.join('');
};

P._cellRO = function(val, isN, fv, col) {
    if (isN) return '<span class="null-value dv-cell">NULL</span>';
    if (fv.length > 200) return '<span class="dv-cell dv-cell-long" data-full="' + esc(fv) + '">' + esc(fv.substring(0, 200)) + '&hellip;</span>';
    if (col.type === 'json' && /^\s*[\[{]/.test(val))
        return '<span class="dv-cell dv-type-json" data-full="' + esc(fv) + '">' + esc(val.length > 80 ? val.substring(0, 80) + '\u2026' : val) + '</span>';
    return '<span class="dv-cell">' + esc(val) + '</span>';
};

// ─── Selection ──────────────────────────────────────────────────────

P.selectCell = function(vr, c) {
    this.anchor = { vr: vr, c: c };
    this.cursor = { vr: vr, c: c };
    this.selRows.clear();
    activeGrid = this;
    this._applySel();
    this._scrollToVR(vr);
};

P.extendSel = function(vr, c) {
    if (!this.anchor) this.anchor = { vr: vr, c: c };
    this.cursor = { vr: vr, c: c };
    this._applySel();
    this._scrollToVR(vr);
};

P.clearSel = function() {
    this.anchor = null;
    this.cursor = null;
    this.selRows.clear();
    this._applySel();
};

P.selectRow = function(vr, shift, ctrl) {
    var di = this.dataIdx(vr);
    if (shift && this.lastClickRow >= 0) {
        var s = Math.min(this.lastClickRow, vr), e = Math.max(this.lastClickRow, vr);
        for (var i = s; i <= e; i++) this.selRows.add(this.dataIdx(i));
    } else if (ctrl) {
        if (this.selRows.has(di)) this.selRows.delete(di); else this.selRows.add(di);
    } else {
        this.selRows.clear();
        this.selRows.add(di);
    }
    this.lastClickRow = vr;
    activeGrid = this;
    this._applySel();
};

P._applySel = function() {
    var tbody = this.table.tBodies[0];
    if (!tbody) return;

    var old = tbody.querySelectorAll('.cell-selected,.cell-range,.dv-row-selected,.row-active');
    for (var i = 0; i < old.length; i++) old[i].classList.remove('cell-selected', 'cell-range', 'dv-row-selected', 'row-active');

    var start, end, offset;
    if (this.virtual) { start = this.vStart; end = this.vEnd; offset = 1; }
    else { start = 0; end = this.viewLen(); offset = 0; }

    var hasRange = this.anchor && this.cursor;
    var minVR, maxVR, minC, maxC;
    if (hasRange) {
        minVR = Math.min(this.anchor.vr, this.cursor.vr);
        maxVR = Math.max(this.anchor.vr, this.cursor.vr);
        minC = Math.min(this.anchor.c, this.cursor.c);
        maxC = Math.max(this.anchor.c, this.cursor.c);
    }

    var rows = tbody.rows;
    for (var vi = start; vi < end; vi++) {
        var domIdx = vi - start + offset;
        var tr = rows[domIdx];
        if (!tr || tr.classList.contains('dv-spacer')) continue;
        var di = this.dataIdx(vi);

        if (this.selRows.has(di)) tr.classList.add('dv-row-selected');
        if (this.cursor && this.cursor.vr === vi) tr.classList.add('row-active');

        if (hasRange && vi >= minVR && vi <= maxVR) {
            var co = this.hasRowNum ? 1 : 0;
            for (var c = minC; c <= maxC; c++) {
                var td = tr.cells[c + co];
                if (!td) continue;
                var span = td.querySelector('.editable-cell, .dv-cell');
                if (!span) continue;
                span.classList.add(vi === this.cursor.vr && c === this.cursor.c ? 'cell-selected' : 'cell-range');
            }
        }
    }
    this._posExplain();
};

// ─── Navigation ─────────────────────────────────────────────────────

P.navigate = function(dvr, dc, shift) {
    if (!this.cursor) { this.selectCell(0, 0); return; }
    var nvr = Math.max(0, Math.min(this.viewLen() - 1, this.cursor.vr + dvr));
    var nc = Math.max(0, Math.min(this.cols.length - 1, this.cursor.c + dc));
    if (nvr === this.cursor.vr && nc === this.cursor.c) return;
    if (shift) this.extendSel(nvr, nc);
    else this.selectCell(nvr, nc);
};

P.moveFromSpan = function(span, dir) {
    var co = this._findCoords(span);
    if (!co) return;
    var dvr = dir === 'down' ? 1 : 0;
    var dc = dir === 'right' ? 1 : dir === 'left' ? -1 : 0;
    var nvr = Math.max(0, Math.min(this.viewLen() - 1, co.vr + dvr));
    var nc = Math.max(0, Math.min(this.cols.length - 1, co.c + dc));
    this.selectCell(nvr, nc);
};

P._scrollToVR = function(vr) {
    if (!this.virtual) {
        var tr = this._getTrForVR(vr);
        if (tr) tr.scrollIntoView({ block: 'nearest' });
        return;
    }
    var targetTop = vr * ROW_H;
    var st = this.wrapper.scrollTop;
    var vh = this.wrapper.clientHeight;
    if (targetTop < st) this.wrapper.scrollTop = targetTop;
    else if (targetTop + ROW_H > st + vh) this.wrapper.scrollTop = targetTop + ROW_H - vh;
};

P._getTrForVR = function(vr) {
    if (!this.virtual) {
        var di = this.dataIdx(vr);
        return this.rows[di] ? this.rows[di]._tr : null;
    }
    if (vr < this.vStart || vr >= this.vEnd) return null;
    return this.table.tBodies[0].rows[vr - this.vStart + 1] || null;
};

P._getCellSpan = function(vr, c) {
    var tr = this._getTrForVR(vr);
    if (!tr) return null;
    var td = tr.cells[c + (this.hasRowNum ? 1 : 0)];
    return td ? td.querySelector('.editable-cell, .dv-cell') : null;
};

P._findCoords = function(el) {
    var tr = el.closest('tr');
    if (!tr) return null;
    var di = parseInt(tr.getAttribute('data-di'));
    if (isNaN(di)) return null;
    var vr = this.viewIdx(di);
    if (vr < 0) return null;
    var td = el.closest('td');
    if (!td || td.classList.contains('row-num') || td.classList.contains('dv-actions'))
        return { vr: vr, c: 0, di: di };
    var c = Array.from(tr.cells).indexOf(td) - this.dataStart;
    if (c < 0) c = 0;
    if (c >= this.cols.length) c = Math.max(0, this.cols.length - 1);
    return { vr: vr, c: c, di: di };
};

// ─── Filtering ──────────────────────────────────────────────────────

P.setGlobalFilter = function(q) {
    this.globalQ = q.toLowerCase().trim();
    this._afterViewChange();
};

P.setColFilter = function(ci, q) {
    if (q) this.colFilters[ci] = q.toLowerCase().trim();
    else delete this.colFilters[ci];
    this._afterViewChange();
};

P._afterViewChange = function() {
    this.clearSel();
    this._buildView();
    if (this.virtual) { this.vStart = -1; this.vEnd = -1; this._renderVis(); }
    else this._renderNV();
    this._updateCount();
};

// ─── Sorting ────────────────────────────────────────────────────────

P.sort = function(ci) {
    if (this.sortCol === ci) this.sortDir = this.sortDir === 'asc' ? 'desc' : 'asc';
    else { this.sortCol = ci; this.sortDir = 'asc'; }

    for (var i = 0; i < this.cols.length; i++) {
        this.cols[i].el.classList.remove('sort-asc', 'sort-desc');
        this.cols[i].el.removeAttribute('data-sort-dir');
    }
    if (ci >= 0 && ci < this.cols.length) {
        this.cols[ci].el.classList.add('sort-' + this.sortDir);
        this.cols[ci].el.setAttribute('data-sort-dir', this.sortDir);
    }
    this._afterViewChange();
};

// ─── Copy ───────────────────────────────────────────────────────────

P.copySelection = function() {
    var text = this._getSelText();
    if (!text) return;
    var self = this;
    navigator.clipboard.writeText(text).then(function() { self._toast('Copied!'); });
};

P._getSelText = function() {
    if (this.selRows.size > 0) {
        var lines = [this.cols.map(function(c) { return c.name; }).join('\t')];
        var self = this;
        this.selRows.forEach(function(di) {
            var r = self.rows[di];
            lines.push(r.full.map(function(v, i) { return r.nulls[i] ? 'NULL' : v; }).join('\t'));
        });
        return lines.join('\n');
    }
    if (!this.anchor || !this.cursor) return '';
    var minVR = Math.min(this.anchor.vr, this.cursor.vr);
    var maxVR = Math.max(this.anchor.vr, this.cursor.vr);
    var minC = Math.min(this.anchor.c, this.cursor.c);
    var maxC = Math.max(this.anchor.c, this.cursor.c);

    if (minVR === maxVR && minC === maxC) {
        var di = this.dataIdx(minVR), r = this.rows[di];
        return r.nulls[minC] ? 'NULL' : r.full[minC];
    }

    var lines = [];
    for (var vr = minVR; vr <= maxVR; vr++) {
        var di = this.dataIdx(vr), r = this.rows[di], cells = [];
        for (var c = minC; c <= maxC; c++) cells.push(r.nulls[c] ? 'NULL' : r.full[c]);
        lines.push(cells.join('\t'));
    }
    return lines.join('\n');
};

// ─── Export ─────────────────────────────────────────────────────────

P.exportData = function(fmt) {
    var hdrs = this.cols.map(function(c) { return c.name; });
    var rows = [], total = this.viewLen();
    for (var i = 0; i < total; i++) {
        var di = this.dataIdx(i), r = this.rows[di];
        rows.push(r.full.map(function(v, j) { return r.nulls[j] ? null : v; }));
    }

    var out = '', mime = 'text/plain', ext = 'txt';
    if (fmt === 'csv') {
        mime = 'text/csv'; ext = 'csv';
        out = hdrs.map(function(h) { return '"' + h.replace(/"/g, '""') + '"'; }).join(',') + '\n';
        rows.forEach(function(r) {
            out += r.map(function(v) { return v === null ? '' : '"' + String(v).replace(/"/g, '""') + '"'; }).join(',') + '\n';
        });
    } else if (fmt === 'json') {
        mime = 'application/json'; ext = 'json';
        out = JSON.stringify(rows.map(function(r) {
            var o = {}; r.forEach(function(v, i) { o[hdrs[i] || 'col' + i] = v; }); return o;
        }), null, 2);
    } else if (fmt === 'sql') {
        ext = 'sql';
        var fn = this.schema ? '"' + this.schema + '"."' + (this.tableName || 'table_name') + '"' : '"' + (this.tableName || 'table_name') + '"';
        rows.forEach(function(r) {
            var vals = r.map(function(v) { return v === null ? 'NULL' : "'" + String(v).replace(/'/g, "''") + "'"; });
            out += 'INSERT INTO ' + fn + ' (' + hdrs.map(function(h) { return '"' + h + '"'; }).join(', ') + ') VALUES (' + vals.join(', ') + ');\n';
        });
    }

    var blob = new Blob([out], { type: mime });
    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url; a.download = 'export.' + ext;
    document.body.appendChild(a); a.click();
    document.body.removeChild(a); URL.revokeObjectURL(url);
};

// ─── Column Resize ──────────────────────────────────────────────────

P._initResize = function() {
    this.el.querySelectorAll('.dv-resize-handle').forEach(function(h) {
        h.addEventListener('mousedown', function(e) {
            e.preventDefault(); e.stopPropagation();
            var th = h.closest('th'), startX = e.clientX, startW = th.offsetWidth;
            function onMove(ev) { var w = Math.max(40, Math.min(600, startW + (ev.clientX - startX))); th.style.width = w + 'px'; th.style.minWidth = w + 'px'; }
            function onUp() { document.removeEventListener('mousemove', onMove); document.removeEventListener('mouseup', onUp); document.body.style.cursor = ''; document.body.style.userSelect = ''; }
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
            document.body.style.cursor = 'col-resize';
            document.body.style.userSelect = 'none';
        });
    });
};

P._autoSize = function() {
    var ruler = document.createElement('span');
    ruler.style.cssText = 'position:absolute;visibility:hidden;white-space:nowrap;font:inherit;padding:0 12px';
    document.body.appendChild(ruler);
    var sample = this.table.querySelector('td');
    if (sample) { var cs = getComputedStyle(sample); ruler.style.fontFamily = cs.fontFamily; ruler.style.fontSize = cs.fontSize; }

    for (var i = 0; i < this.cols.length; i++) {
        var col = this.cols[i];
        ruler.textContent = col.name;
        var hw = ruler.offsetWidth + 40;
        var mx = 0, n = Math.min(this.rows.length, 20);
        for (var r = 0; r < n; r++) {
            ruler.textContent = this.rows[r].cells[i] || '';
            if (ruler.offsetWidth > mx) mx = ruler.offsetWidth;
        }
        var w = Math.max(64, Math.min(360, Math.max(hw, mx + 8)));
        col.el.style.width = w + 'px';
    }
    document.body.removeChild(ruler);
};

// ─── Column Filters ─────────────────────────────────────────────────

P._addColFilters = function() {
    var thead = this.table.tHead;
    if (!thead || thead.rows.length > 1) return;
    var ths = thead.rows[0].cells;
    var fr = document.createElement('tr');
    fr.className = 'dv-filter-row';
    var self = this;

    for (var i = 0; i < ths.length; i++) {
        var td = document.createElement('th');
        td.className = 'dv-col-filter';
        if (ths[i].classList.contains('row-num-header') || ths[i].classList.contains('dv-actions-header')) {
            td.innerHTML = '';
        } else {
            var input = document.createElement('input');
            input.type = 'text';
            input.className = 'dv-col-filter-input';
            input.placeholder = 'Filter\u2026';
            var ci = i - this.dataStart;
            input.setAttribute('data-filter-col', ci);
            (function(c, inp) {
                inp.addEventListener('input', function() { self.setColFilter(c, inp.value); });
            })(ci, input);
            td.appendChild(input);
        }
        fr.appendChild(td);
    }
    thead.appendChild(fr);
};

// ─── Infinite Scroll ────────────────────────────────────────────────

P._setupInfScroll = function() {
    if (!this.editable) return;
    var pg = this.el.querySelector('.dv-pagination');
    if (!pg) return;
    var nb = pg.querySelector('button[hx-get]:last-of-type');
    if (!nb) return;
    var nu = nb.getAttribute('hx-get');
    if (!nu) return;
    var m = nu.match(/page=(\d+).*limit=(\d+)/);
    if (!m) return;

    this.inf.page = parseInt(m[1]) - 1;
    this.inf.limit = parseInt(m[2]);
    this.inf.baseUrl = nu.replace(/page=\d+/, 'page=PAGENUM');
    pg.style.display = 'none';

    var self = this;
    this.wrapper.addEventListener('scroll', function() {
        if (self.inf.loading || self.inf.done) return;
        if (self.wrapper.scrollHeight - self.wrapper.scrollTop - self.wrapper.clientHeight > 200) return;
        self._loadMore();
    });
};

P._loadMore = function() {
    this.inf.loading = true;
    this.inf.page++;
    var self = this;
    var url = this.inf.baseUrl.replace('PAGENUM', this.inf.page);

    fetch(url, { headers: { 'HX-Request': 'true' } })
        .then(function(r) { return r.text(); })
        .then(function(html) {
            var doc = new DOMParser().parseFromString('<div>' + html + '</div>', 'text/html');
            var nt = doc.querySelector('.dv-table tbody');
            if (!nt || nt.rows.length === 0) { self.inf.done = true; self.inf.loading = false; return; }

            var eo = self.hasActions ? 1 : 0;
            while (nt.rows.length > 0) {
                var tr = nt.rows[0];
                var ctid = tr.getAttribute('data-ctid') || '';
                var cells = [], nulls = [], full = [];
                for (var j = self.dataStart; j < tr.cells.length - eo; j++) {
                    var td = tr.cells[j];
                    var span = td.querySelector('.editable-cell, .dv-cell, .null-value');
                    var isN = td.querySelector('.null-value') !== null;
                    var v, fv;
                    if (span) { fv = span.getAttribute('data-full') || span.textContent.trim(); v = span.textContent.trim(); }
                    else { v = td.textContent.trim(); fv = v; }
                    cells.push(v); nulls.push(isN); full.push(fv);
                }
                tr.setAttribute('data-di', self.rows.length);
                self.rows.push({ ctid: ctid, cells: cells, nulls: nulls, full: full, _tr: self.virtual ? null : tr });
                if (!self.virtual) {
                    self.table.tBodies[0].appendChild(tr);
                    if (window.htmx) htmx.process(tr);
                } else { nt.removeChild(tr); }
            }

            self._buildView();
            if (self.virtual) { self.vStart = -1; self.vEnd = -1; self._renderVis(); }
            self._updateCount();
            self.inf.loading = false;
        })
        .catch(function() { self.inf.loading = false; });
};

// ─── Result Diff ────────────────────────────────────────────────────

P._snapshot = function() {
    return {
        cols: this.cols.map(function(c) { return c.name; }),
        rows: this.rows.map(function(r) {
            return { cells: r.cells.slice(), nulls: r.nulls.slice(), full: r.full.slice() };
        })
    };
};

P.toggleDiff = function() {
    if (!this.prevData) return;
    this.diffActive = !this.diffActive;
    if (this.diffActive) this._computeDiff();
    else this.diffMap = null;

    var btn = this.el.querySelector('.dv-diff-btn');
    if (btn) btn.classList.toggle('btn-primary', this.diffActive);

    if (this.virtual) { this.vStart = -1; this.vEnd = -1; this._renderVis(); }
    else this._applyDiffNV();
};

P._computeDiff = function() {
    if (!this.prevData) return;
    this.diffMap = {};
    var prev = this.prevData;
    for (var i = 0; i < this.rows.length; i++) {
        if (i >= prev.rows.length) {
            this.diffMap[i] = { type: 'added', changedCols: new Set() };
            continue;
        }
        var changed = new Set();
        var cur = this.rows[i], old = prev.rows[i];
        for (var c = 0; c < Math.max(cur.full.length, old.full.length); c++) {
            var cv = c < cur.full.length ? cur.full[c] : '';
            var ov = c < old.full.length ? old.full[c] : '';
            if (cv !== ov) changed.add(c);
        }
        this.diffMap[i] = { type: changed.size > 0 ? 'changed' : 'same', changedCols: changed };
    }
};

P._applyDiffNV = function() {
    var tbody = this.table.tBodies[0];
    if (!tbody) return;
    for (var i = 0; i < tbody.rows.length; i++) {
        var tr = tbody.rows[i];
        tr.classList.remove('diff-row-added', 'diff-row-changed', 'diff-row-same');
        var di = parseInt(tr.getAttribute('data-di'));
        if (isNaN(di) || !this.diffMap) continue;
        var d = this.diffMap[di];
        if (!d || d.type === 'same') continue;
        tr.classList.add('diff-row-' + d.type);
        if (d.changedCols.size > 0) {
            var co = this.hasRowNum ? 1 : 0;
            d.changedCols.forEach(function(c) {
                var td = tr.cells[c + co];
                if (td) td.classList.add('diff-cell-changed');
            });
        }
    }
};

// ─── Toolbar ────────────────────────────────────────────────────────

P._enhanceToolbar = function() {
    var tb = this.el.querySelector('.dv-toolbar');
    if (!tb) return;
    var self = this;
    if (this.prevData && this.prevData.rows.length > 0) {
        var btn = document.createElement('button');
        btn.className = 'btn btn-sm btn-ghost dv-diff-btn';
        btn.textContent = 'Diff';
        btn.title = 'Compare with previous result';
        btn.addEventListener('click', function() { self.toggleDiff(); });
        tb.appendChild(btn);
    }
};

P._updateCount = function() {
    var badge = this.el.querySelector('.rows-badge');
    if (badge) {
        var t = this.rows.length, v = this.viewLen();
        badge.textContent = v === t ? t + ' rows' : v + ' / ' + t + ' rows';
    }
    var fc = this.el.querySelector('.dv-filter-count');
    if (fc) fc.textContent = this.viewLen() !== this.rows.length ? this.viewLen() + ' / ' + this.rows.length : '';
};

// ─── Explain Button ─────────────────────────────────────────────────

P._posExplain = function() {
    var btn = window._dvExplainBtn;
    if (!this.cursor) {
        if (btn) btn.style.display = 'none';
        return;
    }
    var span = this._getCellSpan(this.cursor.vr, this.cursor.c);
    if (!span) {
        if (btn) btn.style.display = 'none';
        return;
    }
    if (!btn) {
        btn = document.createElement('button');
        btn.className = 'cell-explain-btn';
        btn.textContent = 'i';
        btn.title = 'Explain This (Ctrl+I)';
        document.body.appendChild(btn);
        window._dvExplainBtn = btn;
        btn.addEventListener('mousedown', function(e) {
            e.preventDefault(); e.stopPropagation();
            if (activeGrid && activeGrid.cursor) {
                var s = activeGrid._getCellSpan(activeGrid.cursor.vr, activeGrid.cursor.c);
                if (s && typeof explainCell === 'function') explainCell(s);
            }
        });
    }
    var td = span.closest('td');
    if (!td) { btn.style.display = 'none'; return; }
    var rect = td.getBoundingClientRect();
    // Hide if cell scrolled out of the scroll container
    var wr = this.wrapper.getBoundingClientRect();
    if (rect.top < wr.top || rect.bottom > wr.bottom) {
        btn.style.display = 'none';
        return;
    }
    btn.style.display = '';
    btn.style.position = 'fixed';
    btn.style.top = (rect.top + 4) + 'px';
    btn.style.left = (rect.right - 26) + 'px';
    btn.style.transform = 'none';
};

// ─── htmx Sync ──────────────────────────────────────────────────────

P._syncHtmx = function(e) {
    var td = e.target && e.target.closest ? e.target.closest('td') : null;
    if (!td) return;
    var tr = td.closest('tr');
    if (!tr) return;
    var di = parseInt(tr.getAttribute('data-di'));
    if (isNaN(di) || di < 0 || di >= this.rows.length) return;
    var ci = Array.from(tr.cells).indexOf(td) - this.dataStart;
    if (ci < 0 || ci >= this.cols.length) return;
    var span = td.querySelector('.editable-cell, .dv-cell, .null-value');
    if (!span) return;
    this.rows[di].cells[ci] = span.textContent.trim();
    this.rows[di].full[ci] = span.getAttribute('data-full') || span.textContent.trim();
    this.rows[di].nulls[ci] = !!td.querySelector('.null-value');
};

P.updateCellValue = function(di, ci, val, isNull) {
    if (di < 0 || di >= this.rows.length || ci < 0 || ci >= this.cols.length) return;
    this.rows[di].full[ci] = val;
    this.rows[di].cells[ci] = val.length > 80 ? val.substring(0, 60) + '\u2026' : val;
    this.rows[di].nulls[ci] = isNull;
};

P._toast = function(msg) {
    var t = document.createElement('div');
    t.className = 'dv-toast';
    t.textContent = msg;
    this.el.appendChild(t);
    setTimeout(function() { t.classList.add('dv-toast-show'); }, 10);
    setTimeout(function() { t.classList.remove('dv-toast-show'); }, 1200);
    setTimeout(function() { t.remove(); }, 1500);
};

// ═══════════════════════════════════════════════════════════════════
//  Event Delegation
// ═══════════════════════════════════════════════════════════════════

function findGrid(el) {
    var dv = el.closest('.data-view');
    return dv ? dv._grid : null;
}

document.addEventListener('click', function(e) {
    if (e.target.closest('.dv-resize-handle')) return;
    if (e.target.closest('.dv-col-filter-input')) return;
    if (e.target.closest('.dv-filter-input')) return;

    // Row number → row selection
    var rn = e.target.closest('.row-num');
    if (rn) {
        var g = findGrid(rn);
        if (!g) return;
        var tr = rn.closest('tr');
        if (!tr) return;
        var di = parseInt(tr.getAttribute('data-di'));
        if (isNaN(di)) return;
        var vr = g.viewIdx(di);
        if (vr < 0) return;
        g.selectRow(vr, e.shiftKey, e.ctrlKey || e.metaKey);
        return;
    }

    // Column sort
    var th = e.target.closest('.dv-table th.sortable');
    if (th && !e.target.closest('.dv-resize-handle')) {
        var g = findGrid(th);
        if (!g) return;
        for (var i = 0; i < g.cols.length; i++) {
            if (g.cols[i].el === th) { g.sort(i); break; }
        }
        return;
    }

    // Cell select
    var cell = e.target.closest('.editable-cell, .dv-cell');
    if (cell) {
        var g = findGrid(cell);
        if (!g) return;
        var co = g._findCoords(cell);
        if (!co) return;
        if (e.shiftKey) g.extendSel(co.vr, co.c);
        else g.selectCell(co.vr, co.c);
        return;
    }

    // Export
    var ex = e.target.closest('[data-dv-export]');
    if (ex) { var g = findGrid(ex); if (g) g.exportData(ex.getAttribute('data-dv-export')); return; }

    // Actions
    var ab = e.target.closest('[data-dv-action]');
    if (ab) {
        var act = ab.getAttribute('data-dv-action');
        var g = findGrid(ab);
        if (act === 'copy' && g) { g.copySelection(); return; }
        if (act === 'toggle-insert' && g) {
            var f = g.el.querySelector('.dv-insert-form');
            if (f) f.style.display = f.style.display === 'none' ? '' : 'none';
            return;
        }
    }

    // Click outside deselects
    if (activeGrid && !e.target.closest('.data-view')) activeGrid.clearSel();
});

document.addEventListener('dblclick', function(e) {
    var cell = e.target.closest('.editable-cell, .dv-cell');
    if (!cell) return;
    if (cell.hasAttribute('hx-get')) return; // htmx handles it
    if (typeof startEdit === 'function') {
        window._dvEditing = true;
        startEdit(cell);
    }
});

document.addEventListener('keydown', function(e) {
    var act = document.activeElement;
    if (act && (act.tagName === 'INPUT' || act.tagName === 'TEXTAREA')) return;

    var g = activeGrid;
    if (!g) return;

    if (!g.cursor && !(e.key === 'a' && (e.ctrlKey || e.metaKey))) return;

    switch (e.key) {
    case 'ArrowUp':    e.preventDefault(); g.navigate(-1, 0, e.shiftKey); break;
    case 'ArrowDown':  e.preventDefault(); g.navigate(1, 0, e.shiftKey); break;
    case 'ArrowLeft':  e.preventDefault(); g.navigate(0, -1, e.shiftKey); break;
    case 'ArrowRight': e.preventDefault(); g.navigate(0, 1, e.shiftKey); break;
    case 'Tab':        e.preventDefault(); g.navigate(0, e.shiftKey ? -1 : 1, false); break;
    case 'Home':       e.preventDefault(); g.selectCell(g.cursor.vr, 0); break;
    case 'End':        e.preventDefault(); g.selectCell(g.cursor.vr, g.cols.length - 1); break;
    case 'PageUp':     e.preventDefault(); g.navigate(-20, 0, e.shiftKey); break;
    case 'PageDown':   e.preventDefault(); g.navigate(20, 0, e.shiftKey); break;
    case 'Enter': case 'F2':
        e.preventDefault();
        var span = g._getCellSpan(g.cursor.vr, g.cursor.c);
        if (span) {
            window._dvEditing = true;
            if (span.hasAttribute('hx-get')) htmx.trigger(span, 'dblclick');
            else if (typeof startEdit === 'function') startEdit(span);
        }
        break;
    case 'Escape':
        g.clearSel();
        break;
    case 'Delete': case 'Backspace':
        if (!g.cursor) break;
        e.preventDefault();
        var span = g._getCellSpan(g.cursor.vr, g.cursor.c);
        if (span && span.classList.contains('editable-cell')) {
            var di = g.dataIdx(g.cursor.vr), row = g.rows[di], col = g.cols[g.cursor.c];
            if (row.ctid && typeof saveCellValue === 'function') {
                var oldV = row.full[g.cursor.c];
                span.textContent = 'NULL';
                span.classList.add('null-value', 'cell-saving');
                saveCellValue(g.db, g.schema, g.tableName, col.name, row.ctid, '', span, oldV);
                g.updateCellValue(di, g.cursor.c, '', true);
            }
        }
        break;
    default:
        if (e.key === 'c' && (e.ctrlKey || e.metaKey)) {
            e.preventDefault(); g.copySelection();
        } else if (e.key === 'a' && (e.ctrlKey || e.metaKey)) {
            e.preventDefault();
            for (var i = 0; i < g.viewLen(); i++) g.selRows.add(g.dataIdx(i));
            g._applySel();
            g._toast('Selected all ' + g.viewLen() + ' rows');
        } else if (e.key === 'i' && (e.ctrlKey || e.metaKey)) {
            e.preventDefault();
            var span = g._getCellSpan(g.cursor.vr, g.cursor.c);
            if (span && typeof explainCell === 'function') explainCell(span);
        } else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && g.cursor) {
            e.preventDefault();
            var span = g._getCellSpan(g.cursor.vr, g.cursor.c);
            if (span) {
                window._dvEditing = true;
                if (span.hasAttribute('hx-get')) htmx.trigger(span, 'dblclick');
                else if (typeof startEdit === 'function') startEdit(span, e.key);
            }
        }
    }
});

document.addEventListener('input', function(e) {
    if (!e.target.classList.contains('dv-filter-input')) return;
    var g = findGrid(e.target);
    if (g) g.setGlobalFilter(e.target.value);
});

// ═══════════════════════════════════════════════════════════════════
//  Init
// ═══════════════════════════════════════════════════════════════════

function initDataViews() {
    document.querySelectorAll('.data-view').forEach(function(el) {
        if (el._grid) return;
        el._grid = new DataGrid(el);
    });
}

if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', initDataViews);
else initDataViews();

document.addEventListener('htmx:afterSwap', function() {
    document.querySelectorAll('.data-view').forEach(function(el) {
        if (el._grid && !document.body.contains(el._grid.table)) el._grid = null;
    });
    initDataViews();
});
document.addEventListener('htmx:afterSettle', initDataViews);

var _obs = new MutationObserver(function(ms) {
    for (var i = 0; i < ms.length; i++) if (ms[i].addedNodes.length > 0) { initDataViews(); return; }
});
_obs.observe(document.body, { childList: true, subtree: true });

window._dvEditing = false;

})();
