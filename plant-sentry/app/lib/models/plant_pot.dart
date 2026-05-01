import 'package:flutter/material.dart';

import '../theme/app_colors.dart';

class PlantPot {
  final String id;
  final String name;
  final String plantType;
  final String emoji;
  final String location; // e.g. "Balcony", "Living Room"
  final double moisture; // 0–100
  final double temperature;
  final double lightLevel; // 0–100
  final MoistureStatus status;
  final List<double> moistureHistory; // last 7 readings
  final DateTime lastWatered;
  final bool autoWaterEnabled;

  PlantPot({
    required this.id,
    required this.name,
    required this.plantType,
    required this.emoji,
    required this.location,
    required this.moisture,
    required this.temperature,
    required this.lightLevel,
    required this.status,
    required this.moistureHistory,
    required this.lastWatered,
    this.autoWaterEnabled = false,
  });
}

enum MoistureStatus { dry, optimal, wet, critical }

extension MoistureStatusExt on MoistureStatus {
  String get label {
    switch (this) {
      case MoistureStatus.dry:
        return 'Khô';
      case MoistureStatus.optimal:
        return 'Tốt';
      case MoistureStatus.wet:
        return 'Ẩm';
      case MoistureStatus.critical:
        return 'Nguy hiểm';
    }
  }

  Color get color {
    switch (this) {
      case MoistureStatus.dry:
        return AppColors.orange;
      case MoistureStatus.optimal:
        return AppColors.green;
      case MoistureStatus.wet:
        return AppColors.blue;
      case MoistureStatus.critical:
        return AppColors.red;
    }
  }

  IconData get icon {
    switch (this) {
      case MoistureStatus.dry:
        return Icons.wb_sunny_outlined;
      case MoistureStatus.optimal:
        return Icons.check_circle_outline;
      case MoistureStatus.wet:
        return Icons.water_drop_outlined;
      case MoistureStatus.critical:
        return Icons.warning_amber_outlined;
    }
  }
}
