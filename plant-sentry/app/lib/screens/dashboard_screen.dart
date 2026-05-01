import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../theme/app_colors.dart';
import '../widgets/alert_banner.dart';
import '../widgets/plant_pot_card.dart';
import '../widgets/plant_pot_grid_card.dart';
import '../widgets/summary_row.dart';
import 'add_plant_screen.dart';
import 'notifications_screen.dart';
import 'plant_detail_screen.dart';

// Responsive helpers

/// Returns the number of grid columns based on available width.
int _gridColsFor(double width) {
  if (width < 350) return 1;
  if (width >= 1200) return 5;
  if (width >= 840) return 4;
  if (width >= 600) return 3;
  return 2;
}

double _gridMainAxisExtentFor(double width, int cols) {
  if (cols <= 1) return 150;
  if (cols == 2) return width < 390 ? 135 : 140;
  if (cols == 3) return 150;
  if (cols == 4) return 155;
  return 160;
}

double _dashboardAppBarHeightFor(double width) {
  if (width >= 1200) return 144;
  if (width >= 840) return 132;
  return 112;
}

double _dashboardContentWidthFor(double width) {
  if (width >= 1400) return 1120;
  return width;
}

// Filter options

enum _FilterTab { all, needsWater, optimal, byLocation }

extension _FilterTabExt on _FilterTab {
  String get label {
    switch (this) {
      case _FilterTab.all:
        return 'Tất cả';
      case _FilterTab.needsWater:
        return 'Cần tưới';
      case _FilterTab.optimal:
        return 'Ổn định';
      case _FilterTab.byLocation:
        return 'Vị trí';
    }
  }

  IconData get icon {
    switch (this) {
      case _FilterTab.all:
        return Icons.local_florist_outlined;
      case _FilterTab.needsWater:
        return Icons.water_drop_outlined;
      case _FilterTab.optimal:
        return Icons.check_circle_outline;
      case _FilterTab.byLocation:
        return Icons.location_on_outlined;
    }
  }
}

class DashboardScreen extends StatefulWidget {
  final List<PlantPot> pots;
  final ValueChanged<PlantPot>? onPotAdded;

  const DashboardScreen({super.key, required this.pots, this.onPotAdded});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  _FilterTab _activeFilter = _FilterTab.all;
  bool _isGridView = true;
  String _searchQuery = '';
  final _searchController = TextEditingController();

  int get alertCount => widget.pots
      .where(
        (p) =>
            p.status == MoistureStatus.dry ||
            p.status == MoistureStatus.critical,
      )
      .length;

  List<PlantPot> get _filteredPots {
    var list = widget.pots;
    if (_searchQuery.isNotEmpty) {
      final q = _searchQuery.toLowerCase();
      list = list
          .where(
            (p) =>
                p.name.toLowerCase().contains(q) ||
                p.plantType.toLowerCase().contains(q) ||
                p.location.toLowerCase().contains(q),
          )
          .toList();
    }
    switch (_activeFilter) {
      case _FilterTab.needsWater:
        return list
            .where(
              (p) =>
                  p.status == MoistureStatus.dry ||
                  p.status == MoistureStatus.critical,
            )
            .toList();
      case _FilterTab.optimal:
        return list.where((p) => p.status == MoistureStatus.optimal).toList();
      case _FilterTab.all:
      case _FilterTab.byLocation:
        return list;
    }
  }

  Map<String, List<PlantPot>> get _potsByLocation {
    final map = <String, List<PlantPot>>{};
    for (final p in _filteredPots) {
      map.putIfAbsent(p.location, () => []).add(p);
    }
    return map;
  }

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final appBarHeight = _dashboardAppBarHeightFor(screenWidth);
    final contentMaxWidth = _dashboardContentWidthFor(screenWidth);

    // Build the pot content slivers based on current filter & view mode
    final potSlivers = _activeFilter == _FilterTab.byLocation
        ? _buildLocationSlivers(screenWidth, contentMaxWidth)
        : _buildPotSlivers(screenWidth, contentMaxWidth);

    return Scaffold(
      backgroundColor: Theme.of(context).colorScheme.surfaceContainerLowest,
      body: CustomScrollView(
        slivers: [
          // ── App bar ──────────────────────────────────────────────
          SliverAppBar(
            expandedHeight: appBarHeight,
            pinned: true,
            floating: false,
            elevation: 0,
            backgroundColor: AppColors.darkGreen,
            foregroundColor: Colors.white,
            title: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                const Text('🌱', style: TextStyle(fontSize: 18)),
                const SizedBox(width: 6),
                const Text(
                  'PlantSense',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                if (alertCount > 0) ...[
                  const SizedBox(width: 8),
                  Container(
                    width: 18,
                    height: 18,
                    decoration: const BoxDecoration(
                      color: AppColors.red,
                      shape: BoxShape.circle,
                    ),
                    child: Center(
                      child: Text(
                        '$alertCount',
                        style: const TextStyle(
                          color: Colors.white,
                          fontSize: 11,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                  ),
                ],
              ],
            ),
            actions: [
              _NotificationButton(
                alertCount: alertCount,
                onPressed: _openNotifications,
              ),
              const _SettingsButton(),
              const SizedBox(width: 4),
            ],
            flexibleSpace: FlexibleSpaceBar(
              collapseMode: CollapseMode.pin,
              background: _DashboardHeader(alertCount: alertCount),
            ),
          ),

          // ── Summary row ──────────────────────────────────────────
          SliverToBoxAdapter(
            child: _centerContent(
              maxWidth: contentMaxWidth,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 16, 16, 0),
                child: SummaryRow(pots: widget.pots),
              ),
            ),
          ),

          // ── Alert banner (conditional) ───────────────────────────
          if (alertCount > 0)
            SliverToBoxAdapter(
              child: _centerContent(
                maxWidth: contentMaxWidth,
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
                  child: AlertBanner(
                    count: alertCount,
                    onViewTap: () => setState(() {
                      _activeFilter = _FilterTab.needsWater;
                      _searchQuery = '';
                      _searchController.clear();
                    }),
                  ),
                ),
              ),
            ),

          // ── Search bar ───────────────────────────────────────────
          SliverToBoxAdapter(
            child: _centerContent(
              maxWidth: contentMaxWidth,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 16, 16, 0),
                child: TextField(
                  controller: _searchController,
                  onChanged: (v) => setState(() => _searchQuery = v),
                  decoration: InputDecoration(
                    hintText: 'Tìm chậu, loại cây, vị trí...',
                    hintStyle: TextStyle(
                      fontSize: 13,
                      color: Colors.grey.shade400,
                    ),
                    prefixIcon: Icon(
                      Icons.search,
                      color: Colors.grey.shade400,
                      size: 20,
                    ),
                    suffixIcon: _searchQuery.isNotEmpty
                        ? IconButton(
                            icon: Icon(
                              Icons.clear,
                              size: 18,
                              color: Colors.grey.shade400,
                            ),
                            onPressed: () {
                              _searchController.clear();
                              setState(() => _searchQuery = '');
                            },
                          )
                        : null,
                    filled: true,
                    fillColor: Theme.of(context).colorScheme.surface,
                    contentPadding: const EdgeInsets.symmetric(
                      vertical: 0,
                      horizontal: 12,
                    ),
                    border: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide(color: Colors.grey.shade200),
                    ),
                    enabledBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: BorderSide(color: Colors.grey.shade200),
                    ),
                    focusedBorder: OutlineInputBorder(
                      borderRadius: BorderRadius.circular(12),
                      borderSide: const BorderSide(
                        color: AppColors.green,
                        width: 1.5,
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ),

          // ── Filter chips + view toggle ───────────────────────────
          SliverToBoxAdapter(
            child: _centerContent(
              maxWidth: contentMaxWidth,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(12, 12, 12, 0),
                child: Row(
                  children: [
                    Expanded(
                      child: SingleChildScrollView(
                        scrollDirection: Axis.horizontal,
                        child: Row(
                          children: _FilterTab.values
                              .map(
                                (tab) => Padding(
                                  padding: const EdgeInsets.only(right: 6),
                                  child: _FilterChip(
                                    tab: tab,
                                    isActive: _activeFilter == tab,
                                    count: _countForTab(tab),
                                    onTap: () =>
                                        setState(() => _activeFilter = tab),
                                  ),
                                ),
                              )
                              .toList(),
                        ),
                      ),
                    ),
                    const SizedBox(width: 8),
                    _ViewToggle(
                      isGrid: _isGridView,
                      onToggle: () =>
                          setState(() => _isGridView = !_isGridView),
                    ),
                  ],
                ),
              ),
            ),
          ),

          // ── Section title + add button ───────────────────────────
          SliverToBoxAdapter(
            child: _centerContent(
              maxWidth: contentMaxWidth,
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 16, 8, 4),
                child: Row(
                  children: [
                    Text(
                      _sectionTitle,
                      style: const TextStyle(
                        fontSize: 15,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const Spacer(),
                    TextButton.icon(
                      onPressed: _navigateToAdd,
                      icon: const Icon(Icons.add, size: 16),
                      label: const Text(
                        'Thêm chậu',
                        style: TextStyle(fontSize: 13),
                      ),
                      style: TextButton.styleFrom(
                        foregroundColor: AppColors.green,
                        padding: const EdgeInsets.symmetric(horizontal: 8),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),

          // ── Pot content (grid / list / location-grouped) ─────────
          ...potSlivers,

          // ── Bottom padding for FAB ───────────────────────────────
          const SliverToBoxAdapter(child: SizedBox(height: 80)),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _navigateToAdd,
        backgroundColor: AppColors.green,
        foregroundColor: Colors.white,
        child: const Icon(Icons.add),
      ),
    );
  }

  // ── Sliver builders ─────────────────────────────────────────────

  /// Builds grid or list slivers for the normal (non-location) view.
  List<Widget> _buildPotSlivers(double screenWidth, double contentMaxWidth) {
    final pots = _filteredPots;

    if (pots.isEmpty) {
      return [const SliverFillRemaining(child: _EmptyState())];
    }

    final cols = _gridColsFor(screenWidth);
    final tileExtent = _gridMainAxisExtentFor(screenWidth, cols);

    if (_isGridView) {
      return [
        SliverPadding(
          padding: EdgeInsets.symmetric(
            horizontal: ((screenWidth - contentMaxWidth) / 2).clamp(
              0.0,
              double.infinity,
            ),
          ),
          sliver: SliverPadding(
            padding: const EdgeInsets.fromLTRB(16, 8, 16, 0),
            sliver: SliverGrid(
              delegate: SliverChildBuilderDelegate(
                (_, i) => PlantPotGridCard(pot: pots[i]),
                childCount: pots.length,
              ),
              gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: cols,
                mainAxisSpacing: 12,
                crossAxisSpacing: 12,
                mainAxisExtent: tileExtent,
              ),
            ),
          ),
        ),
      ];
    }

    // List view
    return [
      SliverPadding(
        padding: EdgeInsets.symmetric(
          horizontal: ((screenWidth - contentMaxWidth) / 2).clamp(
            0.0,
            double.infinity,
          ),
        ),
        sliver: SliverPadding(
          padding: const EdgeInsets.fromLTRB(16, 8, 16, 0),
          sliver: SliverList(
            delegate: SliverChildBuilderDelegate((context, i) {
              if (i == pots.length) {
                return Padding(
                  padding: const EdgeInsets.symmetric(vertical: 16),
                  child: Center(
                    child: Text(
                      '— ${pots.length} chậu —',
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey.shade400,
                      ),
                    ),
                  ),
                );
              }
              return Padding(
                padding: const EdgeInsets.only(bottom: 10),
                child: PlantPotCard(
                  pot: pots[i],
                  onTap: () => Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) => PlantDetailScreen(pot: pots[i]),
                    ),
                  ),
                ),
              );
            }, childCount: pots.length + 1),
          ),
        ),
      ),
    ];
  }

  /// Builds location-grouped slivers (one header + grid/list per location).
  List<Widget> _buildLocationSlivers(
    double screenWidth,
    double contentMaxWidth,
  ) {
    final grouped = _potsByLocation;

    if (grouped.isEmpty) {
      return [const SliverFillRemaining(child: _EmptyState())];
    }

    final cols = _gridColsFor(screenWidth);
    final tileExtent = _gridMainAxisExtentFor(screenWidth, cols);
    final hPad = ((screenWidth - contentMaxWidth) / 2).clamp(
      0.0,
      double.infinity,
    );

    final slivers = <Widget>[];

    grouped.forEach((loc, pots) {
      // Location header
      slivers.add(
        SliverToBoxAdapter(
          child: _centerContent(
            maxWidth: contentMaxWidth,
            child: Padding(
              padding: EdgeInsets.fromLTRB(16 + hPad, 16, 16 + hPad, 10),
              child: Row(
                children: [
                  Container(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 10,
                      vertical: 4,
                    ),
                    decoration: BoxDecoration(
                      color: AppColors.green.withValues(alpha: 0.1),
                      borderRadius: BorderRadius.circular(10),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        const Icon(
                          Icons.location_on,
                          size: 13,
                          color: AppColors.green,
                        ),
                        const SizedBox(width: 4),
                        Text(
                          loc,
                          style: const TextStyle(
                            fontSize: 13,
                            fontWeight: FontWeight.bold,
                            color: AppColors.green,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 8),
                  Text(
                    '${pots.length} chậu',
                    style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
                  ),
                ],
              ),
            ),
          ),
        ),
      );

      // Pots for this location
      if (_isGridView) {
        slivers.add(
          SliverPadding(
            padding: EdgeInsets.fromLTRB(16 + hPad, 0, 16 + hPad, 0),
            sliver: SliverGrid(
              delegate: SliverChildBuilderDelegate(
                (_, j) => PlantPotGridCard(pot: pots[j]),
                childCount: pots.length,
              ),
              gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                crossAxisCount: cols,
                mainAxisSpacing: 10,
                crossAxisSpacing: 10,
                mainAxisExtent: tileExtent,
              ),
            ),
          ),
        );
      } else {
        slivers.add(
          SliverPadding(
            padding: EdgeInsets.fromLTRB(16 + hPad, 0, 16 + hPad, 0),
            sliver: SliverList(
              delegate: SliverChildBuilderDelegate(
                (context, j) => Padding(
                  padding: const EdgeInsets.only(bottom: 10),
                  child: PlantPotCard(
                    pot: pots[j],
                    onTap: () => Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (_) => PlantDetailScreen(pot: pots[j]),
                      ),
                    ),
                  ),
                ),
                childCount: pots.length,
              ),
            ),
          ),
        );
      }
    });

    return slivers;
  }

  // ── Helpers ─────────────────────────────────────────────────────

  /// Centers content up to [maxWidth] horizontally.
  Widget _centerContent({required Widget child, required double maxWidth}) {
    return Align(
      alignment: Alignment.topCenter,
      child: ConstrainedBox(
        constraints: BoxConstraints(maxWidth: maxWidth),
        child: child,
      ),
    );
  }

  String get _sectionTitle {
    final count = _filteredPots.length;
    if (_searchQuery.isNotEmpty) return '$count kết quả';
    switch (_activeFilter) {
      case _FilterTab.all:
        return '$count chậu cây';
      case _FilterTab.needsWater:
        return '$count chậu cần tưới';
      case _FilterTab.optimal:
        return '$count chậu ổn định';
      case _FilterTab.byLocation:
        return '${_potsByLocation.length} khu vực';
    }
  }

  int _countForTab(_FilterTab tab) {
    switch (tab) {
      case _FilterTab.all:
        return widget.pots.length;
      case _FilterTab.needsWater:
        return widget.pots
            .where(
              (p) =>
                  p.status == MoistureStatus.dry ||
                  p.status == MoistureStatus.critical,
            )
            .length;
      case _FilterTab.optimal:
        return widget.pots
            .where((p) => p.status == MoistureStatus.optimal)
            .length;
      case _FilterTab.byLocation:
        return widget.pots.map((p) => p.location).toSet().length;
    }
  }

  void _navigateToAdd() async {
    final result = await Navigator.push<PlantPot>(
      context,
      MaterialPageRoute(builder: (_) => const AddPlantScreen()),
    );
    if (result != null) {
      widget.onPotAdded?.call(result);
    }
  }

  void _openNotifications() async {
    final shouldShowNeedsWater = await Navigator.push<bool>(
      context,
      MaterialPageRoute(builder: (_) => NotificationsScreen(pots: widget.pots)),
    );
    if (shouldShowNeedsWater == true) {
      setState(() {
        _activeFilter = _FilterTab.needsWater;
        _searchQuery = '';
        _searchController.clear();
      });
    }
  }
}

// ── Empty state ────────────────────────────────────────────────────

class _EmptyState extends StatelessWidget {
  const _EmptyState();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Text('🌱', style: TextStyle(fontSize: 56)),
          const SizedBox(height: 16),
          const Text(
            'Không tìm thấy chậu nào',
            style: TextStyle(fontSize: 15, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 6),
          Text(
            'Thử thay đổi bộ lọc hoặc thêm chậu mới',
            style: TextStyle(fontSize: 13, color: Colors.grey.shade500),
          ),
        ],
      ),
    );
  }
}

// ── Filter chip ────────────────────────────────────────────────────

class _FilterChip extends StatelessWidget {
  final _FilterTab tab;
  final bool isActive;
  final int count;
  final VoidCallback onTap;

  const _FilterChip({
    required this.tab,
    required this.isActive,
    required this.count,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 150),
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        decoration: BoxDecoration(
          color: isActive
              ? AppColors.green
              : Theme.of(context).colorScheme.surface,
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: isActive ? AppColors.green : Colors.grey.shade300,
          ),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              tab.icon,
              size: 13,
              color: isActive ? Colors.white : Colors.grey.shade600,
            ),
            const SizedBox(width: 4),
            Text(
              tab.label,
              style: TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w500,
                color: isActive ? Colors.white : Colors.grey.shade700,
              ),
            ),
            const SizedBox(width: 4),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 1),
              decoration: BoxDecoration(
                color: isActive
                    ? Colors.white.withValues(alpha: 0.25)
                    : Colors.grey.shade200,
                borderRadius: BorderRadius.circular(8),
              ),
              child: Text(
                '$count',
                style: TextStyle(
                  fontSize: 10,
                  fontWeight: FontWeight.bold,
                  color: isActive ? Colors.white : Colors.grey.shade600,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ── View toggle ────────────────────────────────────────────────────

class _ViewToggle extends StatelessWidget {
  final bool isGrid;
  final VoidCallback onToggle;

  const _ViewToggle({required this.isGrid, required this.onToggle});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onToggle,
      child: Container(
        padding: const EdgeInsets.all(6),
        decoration: BoxDecoration(
          color: Theme.of(context).colorScheme.surface,
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: Colors.grey.shade300),
        ),
        child: Icon(
          isGrid ? Icons.list_outlined : Icons.grid_view_outlined,
          size: 18,
          color: Colors.grey.shade600,
        ),
      ),
    );
  }
}

// ── Dashboard header background ────────────────────────────────────

class _DashboardHeader extends StatelessWidget {
  final int alertCount;
  const _DashboardHeader({required this.alertCount});

  @override
  Widget build(BuildContext context) {
    final now = DateTime.now();
    final greeting = _greeting(now.hour);
    return LayoutBuilder(
      builder: (context, constraints) {
        final compact = constraints.maxHeight < 124;
        final tight = constraints.maxHeight < 108;
        return Container(
          decoration: const BoxDecoration(
            gradient: LinearGradient(
              begin: Alignment.topLeft,
              end: Alignment.bottomRight,
              colors: [AppColors.darkGreen, AppColors.green],
            ),
          ),
          child: Align(
            alignment: Alignment.bottomLeft,
            child: Padding(
              padding: EdgeInsets.fromLTRB(16, 0, 16, compact ? 10 : 16),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  if (!tight)
                    const Text(
                      'Smart Garden Monitor',
                      style: TextStyle(
                        color: Colors.white60,
                        fontSize: 11,
                        letterSpacing: 0.3,
                      ),
                    ),
                  if (!tight) const SizedBox(height: 6),
                  Row(
                    children: [
                      Text(
                        greeting,
                        style: TextStyle(
                          color: Colors.white,
                          fontSize: compact ? 14 : 15,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                      const SizedBox(width: 8),
                      if (alertCount > 0)
                        Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 10,
                            vertical: 3,
                          ),
                          decoration: BoxDecoration(
                            color: AppColors.red,
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: Text(
                            '$alertCount cảnh báo',
                            style: const TextStyle(
                              color: Colors.white,
                              fontSize: 11,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        )
                      else
                        Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 10,
                            vertical: 3,
                          ),
                          decoration: BoxDecoration(
                            color: Colors.white.withValues(alpha: 0.15),
                            borderRadius: BorderRadius.circular(12),
                          ),
                          child: const Text(
                            'Tất cả ổn ✓',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 11,
                              fontWeight: FontWeight.w500,
                            ),
                          ),
                        ),
                    ],
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }

  String _greeting(int hour) {
    if (hour < 12) return 'Chào buổi sáng 🌄';
    if (hour < 18) return 'Chào buổi chiều ☀️';
    return 'Chào buổi tối 🌙';
  }
}

// ── Action buttons ─────────────────────────────────────────────────

class _NotificationButton extends StatelessWidget {
  final int alertCount;
  final VoidCallback onPressed;
  const _NotificationButton({
    required this.alertCount,
    required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return IconButton(
      icon: Stack(
        clipBehavior: Clip.none,
        children: [
          const Icon(Icons.notifications_outlined, color: Colors.white),
          if (alertCount > 0)
            Positioned(
              top: -3,
              right: -3,
              child: Container(
                width: 14,
                height: 14,
                decoration: const BoxDecoration(
                  color: AppColors.red,
                  shape: BoxShape.circle,
                ),
                child: Center(
                  child: Text(
                    '$alertCount',
                    style: const TextStyle(
                      color: Colors.white,
                      fontSize: 9,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ),
            ),
        ],
      ),
      onPressed: onPressed,
    );
  }
}

class _SettingsButton extends StatelessWidget {
  const _SettingsButton();

  @override
  Widget build(BuildContext context) {
    return IconButton(
      icon: const Icon(Icons.settings_outlined, color: Colors.white),
      onPressed: () => Navigator.pushNamed(context, '/settings'),
    );
  }
}
