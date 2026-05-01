import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../theme/app_colors.dart';

class SummaryRow extends StatelessWidget {
  final List<PlantPot> pots;
  const SummaryRow({super.key, required this.pots});

  @override
  Widget build(BuildContext context) {
    final optimal = pots
        .where((p) => p.status == MoistureStatus.optimal)
        .length;
    final avgMoisture =
        pots.map((p) => p.moisture).reduce((a, b) => a + b) / pots.length;
    return LayoutBuilder(
      builder: (context, constraints) {
        final compact = constraints.maxWidth < 460;
        final spacing = compact ? 8.0 : 12.0;
        return Row(
          children: [
            Expanded(
              child: _StatCard(
                label: 'Tổng chậu',
                value: '${pots.length}',
                icon: Icons.local_florist,
                color: AppColors.green,
                compact: compact,
              ),
            ),
            SizedBox(width: spacing),
            Expanded(
              child: _StatCard(
                label: 'Trạng thái tốt',
                value: '$optimal/${pots.length}',
                icon: Icons.check_circle_outline,
                color: Colors.green.shade700,
                compact: compact,
              ),
            ),
            SizedBox(width: spacing),
            Expanded(
              child: _StatCard(
                label: 'Độ ẩm TB',
                value: '${avgMoisture.toStringAsFixed(0)}%',
                icon: Icons.water_drop_outlined,
                color: Colors.blue.shade600,
                compact: compact,
              ),
            ),
          ],
        );
      },
    );
  }
}

class _StatCard extends StatelessWidget {
  final String label;
  final String value;
  final IconData icon;
  final Color color;
  final bool compact;

  const _StatCard({
    required this.label,
    required this.value,
    required this.icon,
    required this.color,
    this.compact = false,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: EdgeInsets.all(compact ? 10 : 12),
      decoration: BoxDecoration(
        color: Theme.of(
          context,
        ).colorScheme.surfaceContainerHighest.withValues(alpha: 0.5),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: color.withValues(alpha: 0.2), width: 1),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(icon, size: compact ? 16 : 20, color: color),
          SizedBox(height: compact ? 6 : 8),
          Text(
            value,
            style: TextStyle(
              fontSize: compact ? 16 : 20,
              fontWeight: FontWeight.bold,
              color: color,
            ),
          ),
          Text(
            label,
            style: TextStyle(
              fontSize: compact ? 10 : 11,
              color: Colors.grey.shade600,
            ),
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
        ],
      ),
    );
  }
}
