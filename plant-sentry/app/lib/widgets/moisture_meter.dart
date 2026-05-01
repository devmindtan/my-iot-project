import 'dart:math';

import 'package:flutter/material.dart';

import '../models/plant_pot.dart';

class MoistureMeter extends StatelessWidget {
  final double moisture;
  final MoistureStatus status;

  const MoistureMeter({
    super.key,
    required this.moisture,
    required this.status,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: status.color.withValues(alpha: 0.05),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: status.color.withValues(alpha: 0.2)),
      ),
      child: Column(
        children: [
          SizedBox(
            height: 140,
            child: CustomPaint(
              painter: _ArcMeterPainter(
                value: moisture / 100,
                color: status.color,
              ),
              child: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const SizedBox(height: 20),
                    Text(
                      '${moisture.toStringAsFixed(0)}%',
                      style: TextStyle(
                        fontSize: 32,
                        fontWeight: FontWeight.bold,
                        color: status.color,
                      ),
                    ),
                    Text(
                      'Độ ẩm đất',
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey.shade600,
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              _RangeLabel('0%', 'Khô'),
              _RangeLabel('50%', 'Tốt', highlight: true, color: status.color),
              _RangeLabel('100%', 'Ẩm'),
            ],
          ),
        ],
      ),
    );
  }
}

class _RangeLabel extends StatelessWidget {
  final String value;
  final String label;
  final bool highlight;
  final Color? color;

  const _RangeLabel(
    this.value,
    this.label, {
    this.highlight = false,
    this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(
          value,
          style: TextStyle(
            fontSize: 12,
            fontWeight: highlight ? FontWeight.bold : FontWeight.normal,
            color: highlight ? color : Colors.grey.shade500,
          ),
        ),
        Text(
          label,
          style: TextStyle(fontSize: 10, color: Colors.grey.shade500),
        ),
      ],
    );
  }
}

class _ArcMeterPainter extends CustomPainter {
  final double value;
  final Color color;

  _ArcMeterPainter({required this.value, required this.color});

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height - 10);
    final radius = size.width * 0.42;
    const startAngle = pi;
    const sweepAngle = pi;

    final bgPaint = Paint()
      ..color = Colors.grey.shade200
      ..style = PaintingStyle.stroke
      ..strokeWidth = 14
      ..strokeCap = StrokeCap.round;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      sweepAngle,
      false,
      bgPaint,
    );

    final valuePaint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = 14
      ..strokeCap = StrokeCap.round;

    canvas.drawArc(
      Rect.fromCircle(center: center, radius: radius),
      startAngle,
      sweepAngle * value,
      false,
      valuePaint,
    );
  }

  @override
  bool shouldRepaint(_ArcMeterPainter old) =>
      old.value != value || old.color != color;
}
