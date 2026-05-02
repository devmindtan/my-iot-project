import 'dart:convert';
import 'dart:developer' as dev;

import 'package:http/http.dart' as http;

import '../models/plant_pot.dart';

class BackendApi {
  BackendApi._();

  static const String baseUrl = String.fromEnvironment(
    'API_BASE_URL',
    defaultValue: 'http://10.0.2.2:5000',
  );

  static Uri _uri(String path, [Map<String, String>? query]) {
    final root = baseUrl.endsWith('/')
        ? baseUrl.substring(0, baseUrl.length - 1)
        : baseUrl;
    return Uri.parse('$root$path').replace(queryParameters: query);
  }

  static Future<List<ApiNotificationItem>> fetchNotifications({
    int limit = 50,
  }) async {
    final response = await http.get(
      _uri('/api/notifications', {'limit': '$limit'}),
      headers: const {'Content-Type': 'application/json'},
    );
    _throwIfBad(response, 'Failed to fetch notifications');

    final body = jsonDecode(response.body) as Map<String, dynamic>;
    final items = (body['items'] as List<dynamic>? ?? const [])
        .whereType<Map<String, dynamic>>()
        .map(ApiNotificationItem.fromJson)
        .toList();

    return items;
  }

  static Future<List<ApiWateringLogItem>> fetchWateringLogs({
    int limit = 30,
  }) async {
    final response = await http.get(
      _uri('/api/watering-logs', {'limit': '$limit'}),
      headers: const {'Content-Type': 'application/json'},
    );
    _throwIfBad(response, 'Failed to fetch watering logs');

    final body = jsonDecode(response.body) as Map<String, dynamic>;
    final items = (body['items'] as List<dynamic>? ?? const [])
        .whereType<Map<String, dynamic>>()
        .map(ApiWateringLogItem.fromJson)
        .toList();

    return items;
  }

  static Future<void> markNotificationRead(int id) async {
    final response = await http.patch(
      _uri('/api/notifications/$id/read'),
      headers: const {'Content-Type': 'application/json'},
      body: jsonEncode({'is_read': true}),
    );
    _throwIfBad(response, 'Failed to mark notification as read');
  }

  static Future<void> markAllNotificationsRead() async {
    final response = await http.patch(
      _uri('/api/notifications/read-all'),
      headers: const {'Content-Type': 'application/json'},
    );
    _throwIfBad(response, 'Failed to mark all notifications as read');
  }

  static Future<List<ApiDashboardPot>> fetchDashboard() async {
    final uri = _uri('/api/dashboard');
    dev.log('[BackendApi] GET $uri', name: 'BackendApi');

    final response = await http.get(
      uri,
      headers: const {'Content-Type': 'application/json'},
    );

    dev.log(
      '[BackendApi] dashboard → ${response.statusCode}',
      name: 'BackendApi',
    );

    _throwIfBad(response, 'Failed to fetch dashboard');

    final body = jsonDecode(response.body) as Map<String, dynamic>;
    final pots = (body['pots'] as List<dynamic>? ?? const [])
        .whereType<Map<String, dynamic>>()
        .map(ApiDashboardPot.fromJson)
        .toList();

    dev.log(
      '[BackendApi] dashboard returned ${pots.length} pots',
      name: 'BackendApi',
    );
    return pots;
  }

  static void _throwIfBad(http.Response response, String fallbackMessage) {
    if (response.statusCode >= 200 && response.statusCode < 300) {
      return;
    }

    try {
      final body = jsonDecode(response.body) as Map<String, dynamic>;
      final error = body['error']?.toString();
      throw Exception(error == null || error.isEmpty ? fallbackMessage : error);
    } catch (_) {
      throw Exception(fallbackMessage);
    }
  }
}

class ApiNotificationItem {
  final int id;
  final String userId;
  final String? potId;
  final String type;
  final String title;
  final String body;
  bool isRead;
  final DateTime createdAt;

  ApiNotificationItem({
    required this.id,
    required this.userId,
    required this.potId,
    required this.type,
    required this.title,
    required this.body,
    required this.isRead,
    required this.createdAt,
  });

  factory ApiNotificationItem.fromJson(Map<String, dynamic> json) {
    return ApiNotificationItem(
      id: (json['id'] as num?)?.toInt() ?? 0,
      userId: json['user_id']?.toString() ?? '',
      potId: json['pot_id']?.toString(),
      type: json['type']?.toString() ?? 'info',
      title: json['title']?.toString() ?? '',
      body: json['body']?.toString() ?? '',
      isRead: json['is_read'] == true,
      createdAt:
          DateTime.tryParse(json['created_at']?.toString() ?? '') ??
          DateTime.now(),
    );
  }
}

class ApiWateringLogItem {
  final int id;
  final String potId;
  final String triggeredBy;
  final int? durationSeconds;
  final double? moistureBefore;
  final double? moistureAfter;
  final DateTime createdAt;
  final String? potName;

  ApiWateringLogItem({
    required this.id,
    required this.potId,
    required this.triggeredBy,
    required this.durationSeconds,
    required this.moistureBefore,
    required this.moistureAfter,
    required this.createdAt,
    required this.potName,
  });

  factory ApiWateringLogItem.fromJson(Map<String, dynamic> json) {
    final pot = json['pot'] is Map<String, dynamic>
        ? json['pot'] as Map<String, dynamic>
        : <String, dynamic>{};

    return ApiWateringLogItem(
      id: (json['id'] as num?)?.toInt() ?? 0,
      potId: json['pot_id']?.toString() ?? '',
      triggeredBy: json['triggered_by']?.toString() ?? 'manual',
      durationSeconds: (json['duration_seconds'] as num?)?.toInt(),
      moistureBefore: (json['moisture_before'] as num?)?.toDouble(),
      moistureAfter: (json['moisture_after'] as num?)?.toDouble(),
      createdAt:
          DateTime.tryParse(json['created_at']?.toString() ?? '') ??
          DateTime.now(),
      potName: pot['name']?.toString(),
    );
  }
}

/// Pot data returned by GET /api/dashboard (includes latest sensor readings).
class ApiDashboardPot {
  final String id;
  final String name;
  final String plantType;
  final String emoji;
  final String location;
  final bool autoWaterEnabled;
  final int waterThreshold;
  final String status; // 'critical' | 'dry' | 'optimal' | 'wet' | 'unknown'
  final double? moisture;
  final double? temperature;
  final double? lightLevel;
  final DateTime? recordedAt;

  ApiDashboardPot({
    required this.id,
    required this.name,
    required this.plantType,
    required this.emoji,
    required this.location,
    required this.autoWaterEnabled,
    required this.waterThreshold,
    required this.status,
    required this.moisture,
    required this.temperature,
    required this.lightLevel,
    required this.recordedAt,
  });

  factory ApiDashboardPot.fromJson(Map<String, dynamic> json) {
    final latest = json['latest'] is Map<String, dynamic>
        ? json['latest'] as Map<String, dynamic>
        : <String, dynamic>{};

    return ApiDashboardPot(
      id: json['id']?.toString() ?? '',
      name: json['name']?.toString() ?? '',
      plantType: json['plant_type']?.toString() ?? '',
      emoji: json['emoji']?.toString() ?? '🌿',
      location: json['location']?.toString() ?? '',
      autoWaterEnabled: json['auto_water_enabled'] == true,
      waterThreshold: (json['water_threshold'] as num?)?.toInt() ?? 30,
      status: json['status']?.toString() ?? 'unknown',
      moisture: (latest['moisture'] as num?)?.toDouble(),
      temperature: (latest['temperature'] as num?)?.toDouble(),
      lightLevel: (latest['light_level'] as num?)?.toDouble(),
      recordedAt: DateTime.tryParse(latest['recorded_at']?.toString() ?? ''),
    );
  }

  PlantPot toPlantPot() {
    final moistureValue = moisture ?? 0.0;
    final statusValue = switch (status) {
      'critical' => MoistureStatus.critical,
      'dry' => MoistureStatus.dry,
      'wet' => MoistureStatus.wet,
      _ => moistureValue == 0 ? MoistureStatus.dry : MoistureStatus.optimal,
    };

    return PlantPot(
      id: id,
      name: name,
      plantType: plantType,
      emoji: emoji,
      location: location,
      moisture: moistureValue,
      temperature: temperature ?? 0.0,
      lightLevel: lightLevel ?? 0.0,
      status: statusValue,
      moistureHistory: moistureValue > 0 ? [moistureValue] : [],
      lastWatered: recordedAt ?? DateTime.now(),
      autoWaterEnabled: autoWaterEnabled,
    );
  }
}
