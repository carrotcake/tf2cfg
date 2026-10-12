# `ConsoleCommandModel::suggest()` Optimization Design

**Goal:** Fastest possible correct implementation for substring ("contains") matching with three orthogonal filters (context, platform, cheat), on a static catalog of ~3800 commands.

**Current cost:** 3800 × 3 filter checks + `contains()` per keystroke ≈ 15-20 µs on modern CPU. Acceptable but wasteful; called 30-60×/sec while typing.

---

## Constraints & Requirements

1. **Static catalog**: `m_all` never changes after `setCommands()` (once at startup).
2. **Three filters**: context (7 values + "all"), platform (5 values + "all"), cheat (bool). Changed rarely (dropdown clicks).
3. **Query**: case-insensitive substring match on `name`. Typical length 3-12 chars.
4. **Limit**: caller caps results (default 20).
5. **Sorted output**: results must follow the model's sort order (alnum key, then case-insensitive name).
6. **Correctness**: must respect all active filters exactly as `rebuild()` does.

---

## Candidate Approaches (ranked by query speed)

| Approach | Query time | Build time | Memory | Complexity | Notes |
|----------|------------|------------|--------|------------|-------|
| **A. Trigram index per filter combo** | O(k) trigrams → intersect → verify | O(N) per combo | ~200 KB/combo | Medium | Optimal for substring; filters handled by separate indices |
| **B. Suffix array + filter bitsets** | O(m log N) + bitset AND | O(N log N) | ~200 KB | High | Overkill for N=3800 |
| **C. Filtered vector cache + linear scan** | O(F) where F = filtered count | O(N) per combo | ~F × ptr | **Trivial** | F ≤ 500 typical; 500 × contains = ~5 µs |
| **D. Full trigram index (no filter separation)** | O(k) → intersect → filter verify | O(N) | ~200 KB | Low | Filter check after candidate retrieval |
| **E. Debounce only (QML)** | Same per call, fewer calls | None | None | None | Doesn't fix per-call cost |

---

## Recommended: Approach C (Filtered Vector Cache) — Simplest, Fast Enough

### Why not trigram (A/D)?
- N=3800 is tiny. A linear scan of the **filtered** set (typically 200-600 entries) takes **3-8 µs** — faster than the overhead of trigram intersection + candidate verification + filter re-check.
- Filter combinations actually used: user picks one context + one platform + cheat toggle. At most a few combos per session.
- Trigram adds ~200 lines of code, subtle bugs (Unicode, case-folding, empty query), and maintenance burden.

### Design

```cpp
// In ConsoleCommandModel private:
struct FilterKey {
    QString context;
    QString platform;
    bool showCheats;
    bool operator==(const FilterKey&) const = default;
};

struct FilterCache {
    QVector<int> visibleIndices;  // indices into m_all, already sorted
    // Optional: trigram index if profiling shows linear scan too slow
};

QHash<FilterKey, FilterCache, FilterKeyHasher> m_filterCache;
FilterKey m_currentFilterKey;

void rebuild() {
    m_currentFilterKey = {m_contextFilter, m_platformFilter, m_showCheats};
    auto it = m_filterCache.find(m_currentFilterKey);
    if (it == m_filterCache.end()) {
        // Build once per unique filter combo
        FilterCache cache;
        QVector<QPair<QString, int>> keyed;
        keyed.reserve(m_all.size());
        for (int i = 0; i < m_all.size(); ++i) {
            const auto *cmd = m_all[i];
            if (!m_contextFilter.isEmpty() && cmd->get_context() != m_contextFilter) continue;
            if (!m_platformFilter.isEmpty() && cmd->get_platform() != m_platformFilter) continue;
            if (!m_showCheats && cmd->isCheat()) continue;
            keyed.append({commandSortKey(cmd->get_name()), i});
        }
        std::sort(keyed.begin(), keyed.end(),
                  [this](const auto& a, const auto& b) {
                      if (a.first != b.first) return a.first < b.first;
                      return QString::compare(m_all[a.second]->get_name(),
                                              m_all[b.second]->get_name(),
                                              Qt::CaseInsensitive) < 0;
                  });
        cache.visibleIndices.resize(keyed.size());
        for (int r = 0; r < keyed.size(); ++r) cache.visibleIndices[r] = keyed[r].second;
        m_filterCache.insert(m_currentFilterKey, std::move(cache));
    }
    m_visible = m_filterCache[m_currentFilterKey].visibleIndices;
}

QStringList suggest(const QString& text, int limit) const {
    QStringList out;
    if (text.isEmpty()) return out;

    // Fast path: current filter combo cached
    auto it = m_filterCache.find(m_currentFilterKey);
    if (it == m_filterCache.end()) return out; // no visible entries

    const QVector<int>& candidates = it->visibleIndices;
    const QString needle = text.toLower(); // case-insensitive

    for (int idx : candidates) {
        if (m_all[idx]->get_name().contains(needle, Qt::CaseInsensitive)) {
            out.append(m_all[idx]->get_name());
            if (out.size() >= limit) break;
        }
    }
    return out;
}
```

### Performance

| Filter combo | Filtered count (est.) | Scan time | vs current |
|--------------|----------------------|-----------|------------|
| client controls / universal / cheats on | ~500 | ~4 µs | **5× faster** |
| all / all / cheats on | 3800 | ~20 µs | same (worst case) |
| client controls / universal / cheats off | ~400 | ~3 µs | **7× faster** |

### Correctness guarantees

- `rebuild()` and `suggest()` use **identical filter logic** (shared `if` chain).
- Sort order matches `rebuild()` exactly (same `commandSortKey` + tie-breaker).
- Cache keyed by the three filter values — changing any filter auto-rebuilds.
- `m_visible` still populated for `rowCount`/`data` (model roles unchanged).

---

## Alternative: Approach A (Trigram Index) — If Scale Grows

Only implement if profiling shows Approach C is a bottleneck (unlikely at N=3800).

### Trigram index structure

```cpp
struct TrigramIndex {
    // Map trigram (lowercase 3-char) -> sorted vector of indices into m_all
    QHash<QString, QVector<int>> postings;

    void build(const QVector<cmd::ConVar*>& all) { ... }
    QVector<int> intersect(const QStringList& trigrams) const { ... }
};
```

### Per-filter-combo index

```cpp
struct FilterCache {
    QVector<int> visibleIndices;  // for model roles
    TrigramIndex trigram;         // for suggest()
};
```

### Query algorithm

```
1. Generate trigrams from query (lowercase, length-3 substrings).
   "attack" → ["att", "tta", "tac", "ack"]
2. Retrieve posting lists for each trigram.
3. Intersect (smallest list first).
4. Verify each candidate: filters + full contains() check.
5. Sort verified candidates by model sort order (or maintain sort in postings).
6. Take first `limit`.
```

### Trade-offs

- **Pros**: Substring search independent of filtered set size.
- **Cons**: ~150 extra lines; Unicode case-folding tricky; must keep postings sorted by model order; filter verification still needed per candidate.

---

## Implementation Checklist (Approach C)

- [ ] Add `FilterKey` struct with `operator==` and `qHash` specialization.
- [ ] Add `m_filterCache: QHash<FilterKey, FilterCache>` member.
- [ ] Modify `rebuild()` to populate/return cached `visibleIndices`.
- [ ] Modify `suggest()` to iterate `m_filterCache[m_currentFilterKey].visibleIndices`.
- [ ] Ensure `setContextFilter`/`setPlatformFilter`/`setShowCheats` invalidate `m_currentFilterKey` (already done via `rebuild()`).
- [ ] Benchmark: `suggest("att", 20)` with various filters — target < 10 µs.

---

## What NOT to Do

- ❌ Debounce in QML only — hides the problem, doesn't fix it.
- ❌ Full suffix array — O(N log N) build, complex filter integration.
- ❌ Trigram without filter separation — still verifies filters per candidate.
- ❌ Precompute all 70 filter combos at startup — wasteful; build on demand.

---

## Verdict

**Implement Approach C (Filtered Vector Cache).** It is:
- **Correct**: identical filter logic to `rebuild()`.
- **Fast**: 3-8 µs typical, 20 µs worst case.
- **Simple**: ~30 lines added, no new data structures.
- **Maintainable**: obvious logic, easy to audit.

If future data grows to >50k entries (won't happen for TF2), revisit trigram.