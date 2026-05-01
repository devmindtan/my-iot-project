import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../theme/app_colors.dart';
import '../widgets/future_features_banner.dart';
import '../widgets/moisture_chart.dart';
import '../widgets/moisture_meter.dart';

class PlantDetailScreen extends StatefulWidget {
  final PlantPot pot;
  const PlantDetailScreen({super.key, required this.pot});

  @override
  State<PlantDetailScreen> createState() => _PlantDetailScreenState();
}

class _PlantDetailScreenState extends State<PlantDetailScreen> {
  bool _autoWater = false;

  @override
  void initState() {
    super.initState();
    _autoWater = widget.pot.autoWaterEnabled;
  }

  @override
  Widget build(BuildContext context) {
    final pot = widget.pot;
    final width = MediaQuery.of(context).size.width;
    final appBarHeight = width >= 1200
        ? 240.0
        : width >= 900
        ? 210.0
        : width >= 600
        ? 185.0
        : 164.0;
    final contentMaxWidth = width >= 1200
        ? 980.0
        : width >= 900
        ? 820.0
        : 620.0;
    final horizontalPadding = ((width - contentMaxWidth) / 2)
        .clamp(16.0, 40.0)
        .toDouble();
    return Scaffold(
      body: CustomScrollView(
        slivers: [
          // ── Hero App Bar ──
          SliverAppBar(
            expandedHeight: appBarHeight,
            pinned: true,
            flexibleSpace: FlexibleSpaceBar(
              title: Text(
                pot.name,
                style: const TextStyle(fontWeight: FontWeight.bold),
              ),
              background: Container(
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    begin: Alignment.topCenter,
                    end: Alignment.bottomCenter,
                    colors: [
                      pot.status.color,
                      pot.status.color.withValues(alpha: 0.7),
                    ],
                  ),
                ),
                child: Center(
                  child: Text(pot.emoji, style: const TextStyle(fontSize: 80)),
                ),
              ),
            ),
          ),

          SliverPadding(
            padding: EdgeInsets.symmetric(
              horizontal: horizontalPadding,
              vertical: 16,
            ),
            sliver: SliverList(
              delegate: SliverChildListDelegate([
                // Plant type chips
                Row(
                  children: [
                    _Chip(label: pot.plantType, color: pot.status.color),
                    const SizedBox(width: 8),
                    _Chip(
                      label: pot.status.label,
                      color: pot.status.color,
                      icon: pot.status.icon,
                    ),
                  ],
                ),

                const SizedBox(height: 20),

                // ── Moisture Big Display ──
                MoistureMeter(moisture: pot.moisture, status: pot.status),

                const SizedBox(height: 20),

                // ── Sensor Grid ──
                const Text(
                  'Thông số cảm biến',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 12),
                LayoutBuilder(
                  builder: (context, constraints) {
                    final gridWidth = constraints.maxWidth;
                    final crossAxisCount = gridWidth >= 860
                        ? 4
                        : gridWidth >= 620
                        ? 3
                        : 2;
                    final ratio = crossAxisCount >= 4
                        ? 1.55
                        : crossAxisCount == 3
                        ? 1.48
                        : 1.6;
                    return GridView.count(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      crossAxisCount: crossAxisCount,
                      childAspectRatio: ratio,
                      mainAxisSpacing: 12,
                      crossAxisSpacing: 12,
                      children: [
                        _SensorCard(
                          label: 'Độ ẩm đất',
                          value: '${pot.moisture.toStringAsFixed(0)}%',
                          icon: Icons.water_drop,
                          color: Colors.blue.shade600,
                          subtitle: 'Mức tối ưu: 50–70%',
                        ),
                        _SensorCard(
                          label: 'Nhiệt độ',
                          value: '${pot.temperature}°C',
                          icon: Icons.thermostat,
                          color: Colors.orange.shade600,
                          subtitle: 'Bình thường',
                        ),
                        _SensorCard(
                          label: 'Ánh sáng',
                          value: '${pot.lightLevel.toStringAsFixed(0)}%',
                          icon: Icons.wb_sunny,
                          color: Colors.amber.shade600,
                          subtitle: 'Cường độ hiện tại',
                        ),
                        _SensorCard(
                          label: 'Tưới lần cuối',
                          value: _formatTime(pot.lastWatered),
                          icon: Icons.history,
                          color: Colors.teal.shade600,
                          subtitle: 'Thời gian trước',
                        ),
                      ],
                    );
                  },
                ),

                const SizedBox(height: 20),

                // ── Moisture History Chart ──
                const Text(
                  'Lịch sử độ ẩm (7 ngày)',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 12),
                MoistureChart(
                  data: pot.moistureHistory,
                  color: pot.status.color,
                ),

                const SizedBox(height: 20),

                // ── Controls ──
                const Text(
                  'Điều khiển',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 12),

                // Auto water toggle
                Container(
                  padding: const EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    color: Theme.of(context).colorScheme.surface,
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: Colors.grey.shade200),
                  ),
                  child: Row(
                    children: [
                      Icon(Icons.autorenew, color: Colors.blue.shade600),
                      const SizedBox(width: 12),
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const Text(
                              'Tưới tự động',
                              style: TextStyle(fontWeight: FontWeight.w600),
                            ),
                            Text(
                              'Tưới khi độ ẩm dưới 40%',
                              style: TextStyle(
                                fontSize: 12,
                                color: Colors.grey.shade600,
                              ),
                            ),
                          ],
                        ),
                      ),
                      Switch(
                        value: _autoWater,
                        onChanged: (v) => setState(() => _autoWater = v),
                        activeColor: AppColors.green,
                      ),
                    ],
                  ),
                ),

                const SizedBox(height: 12),

                // Water now button
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton.icon(
                    onPressed: () => _showWaterConfirm(context),
                    icon: const Icon(Icons.water_drop),
                    label: const Text(
                      'Tưới ngay bây giờ',
                      style: TextStyle(fontSize: 16),
                    ),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppColors.green,
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(12),
                      ),
                    ),
                  ),
                ),

                const SizedBox(height: 20),

                // ── Future Features ──
                const FutureFeaturesBanner(),

                const SizedBox(height: 32),
              ]),
            ),
          ),
        ],
      ),
    );
  }

  String _formatTime(DateTime dt) {
    final diff = DateTime.now().difference(dt);
    if (diff.inHours < 1) return '${diff.inMinutes} phút';
    if (diff.inHours < 24) return '${diff.inHours} giờ';
    return '${diff.inDays} ngày';
  }

  void _showWaterConfirm(BuildContext context) {
    showModalBottomSheet(
      context: context,
      shape: const RoundedRectangleBorder(
        borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
      ),
      builder: (_) => Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text('💧', style: TextStyle(fontSize: 48)),
            const SizedBox(height: 12),
            const Text(
              'Tưới nước ngay?',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 8),
            Text(
              'Hệ thống sẽ tưới ${widget.pot.name} trong 30 giây.',
              textAlign: TextAlign.center,
              style: TextStyle(color: Colors.grey.shade600),
            ),
            const SizedBox(height: 24),
            Row(
              children: [
                Expanded(
                  child: OutlinedButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Hủy'),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton(
                    onPressed: () {
                      Navigator.pop(context);
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(
                          content: Text('✅ Đang tưới nước...'),
                          backgroundColor: AppColors.green,
                        ),
                      );
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppColors.green,
                      foregroundColor: Colors.white,
                    ),
                    child: const Text('Xác nhận'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _Chip extends StatelessWidget {
  final String label;
  final Color color;
  final IconData? icon;

  const _Chip({required this.label, required this.color, this.icon});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (icon != null) ...[
            Icon(icon, size: 14, color: color),
            const SizedBox(width: 4),
          ],
          Text(
            label,
            style: TextStyle(color: color, fontWeight: FontWeight.w600),
          ),
        ],
      ),
    );
  }
}

class _SensorCard extends StatelessWidget {
  final String label;
  final String value;
  final IconData icon;
  final Color color;
  final String subtitle;

  const _SensorCard({
    required this.label,
    required this.value,
    required this.icon,
    required this.color,
    required this.subtitle,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withValues(alpha: 0.2)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, size: 16, color: color),
              const SizedBox(width: 6),
              Text(
                label,
                style: TextStyle(fontSize: 11, color: Colors.grey.shade600),
              ),
            ],
          ),
          const Spacer(),
          Text(
            value,
            style: TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
              color: color,
            ),
          ),
          Text(
            subtitle,
            style: TextStyle(fontSize: 10, color: Colors.grey.shade500),
          ),
        ],
      ),
    );
  }
}
