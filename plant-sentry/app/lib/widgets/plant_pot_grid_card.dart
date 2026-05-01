import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../screens/plant_detail_screen.dart';

/// Compact card designed for 2-column grid — scales well for 20+ pots.
class PlantPotGridCard extends StatelessWidget {
  final PlantPot pot;

  const PlantPotGridCard({super.key, required this.pot});

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final width = constraints.maxWidth;
        final dense = width < 175;
        final roomy = width >= 210;
        final padding = roomy
            ? 12.0
            : dense
            ? 7.0
            : 10.0;
        final emojiSize = roomy
            ? 28.0
            : dense
            ? 20.0
            : 24.0;
        final titleSize = roomy
            ? 13.0
            : dense
            ? 10.5
            : 12.0;
        final bodySize = roomy
            ? 11.0
            : dense
            ? 9.0
            : 10.0;
        final iconSize = roomy ? 12.0 : 11.0;

        return GestureDetector(
          onTap: () => Navigator.push(
            context,
            MaterialPageRoute(builder: (_) => PlantDetailScreen(pot: pot)),
          ),
          child: Container(
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.surface,
              borderRadius: BorderRadius.circular(14),
              border: Border.all(
                color: pot.status.color.withValues(alpha: 0.35),
                width: 1.5,
              ),
              boxShadow: [
                BoxShadow(
                  color: Colors.black.withValues(alpha: 0.04),
                  blurRadius: 6,
                  offset: const Offset(0, 2),
                ),
              ],
            ),
            child: Padding(
              padding: EdgeInsets.all(padding),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                mainAxisSize: MainAxisSize.min,
                children: [
                  Row(
                    children: [
                      Text(pot.emoji, style: TextStyle(fontSize: emojiSize)),
                      const Spacer(),
                      _StatusDot(status: pot.status, compact: dense),
                    ],
                  ),
                  SizedBox(height: dense ? 4 : 6),
                  Text(
                    pot.name,
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                      fontSize: titleSize,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  Text(
                    pot.plantType,
                    style: TextStyle(
                      fontSize: bodySize,
                      color: Colors.grey.shade500,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  SizedBox(height: dense ? 3 : 4),
                  Row(
                    children: [
                      Icon(
                        Icons.location_on_outlined,
                        size: iconSize,
                        color: Colors.grey.shade400,
                      ),
                      const SizedBox(width: 2),
                      Expanded(
                        child: Text(
                          pot.location,
                          style: TextStyle(
                            fontSize: bodySize,
                            color: Colors.grey.shade400,
                          ),
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: dense ? 5 : 6),
                  Row(
                    children: [
                      Icon(
                        Icons.water_drop,
                        size: iconSize,
                        color: pot.status.color,
                      ),
                      const SizedBox(width: 4),
                      Expanded(
                        child: ClipRRect(
                          borderRadius: BorderRadius.circular(3),
                          child: LinearProgressIndicator(
                            value: pot.moisture / 100,
                            minHeight: dense ? 4 : 5,
                            backgroundColor: Colors.grey.shade200,
                            valueColor: AlwaysStoppedAnimation<Color>(
                              pot.status.color,
                            ),
                          ),
                        ),
                      ),
                      const SizedBox(width: 6),
                      Text(
                        '${pot.moisture.toStringAsFixed(0)}%',
                        style: TextStyle(
                          fontSize: bodySize,
                          fontWeight: FontWeight.bold,
                          color: pot.status.color,
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: dense ? 3 : 4),
                  Row(
                    children: [
                      Icon(
                        Icons.thermostat_outlined,
                        size: iconSize,
                        color: Colors.orange.shade400,
                      ),
                      const SizedBox(width: 2),
                      Text(
                        '${pot.temperature}°',
                        style: TextStyle(
                          fontSize: bodySize,
                          color: Colors.grey.shade600,
                        ),
                      ),
                      SizedBox(width: dense ? 6 : 8),
                      Icon(
                        Icons.wb_sunny_outlined,
                        size: iconSize,
                        color: Colors.amber.shade600,
                      ),
                      const SizedBox(width: 2),
                      Text(
                        '${pot.lightLevel.toStringAsFixed(0)}%',
                        style: TextStyle(
                          fontSize: bodySize,
                          color: Colors.grey.shade600,
                        ),
                      ),
                      if (pot.autoWaterEnabled) ...[
                        const Spacer(),
                        Icon(
                          Icons.autorenew,
                          size: roomy ? 14 : 12,
                          color: Colors.blue.shade400,
                        ),
                      ],
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
}

class _StatusDot extends StatelessWidget {
  final MoistureStatus status;
  final bool compact;
  const _StatusDot({required this.status, this.compact = false});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
      decoration: BoxDecoration(
        color: status.color.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 6,
            height: 6,
            decoration: BoxDecoration(
              color: status.color,
              shape: BoxShape.circle,
            ),
          ),
          if (!compact) ...[
            const SizedBox(width: 4),
            Text(
              status.label,
              style: TextStyle(
                fontSize: 10,
                color: status.color,
                fontWeight: FontWeight.w600,
              ),
            ),
          ],
        ],
      ),
    );
  }
}
