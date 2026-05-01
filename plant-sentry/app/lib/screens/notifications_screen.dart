import 'package:flutter/material.dart';

import '../models/plant_pot.dart';
import '../theme/app_colors.dart';

class NotificationsScreen extends StatefulWidget {
  final List<PlantPot> pots;
  const NotificationsScreen({super.key, required this.pots});

  @override
  State<NotificationsScreen> createState() => _NotificationsScreenState();
}

class _NotificationsScreenState extends State<NotificationsScreen> {
  final List<_Notification> _notifications = [];

  @override
  void initState() {
    super.initState();
    _buildNotifications();
  }

  void _buildNotifications() {
    for (final pot in widget.pots) {
      if (pot.status == MoistureStatus.dry) {
        _notifications.add(
          _Notification(
            type: _NotifType.warning,
            title: '${pot.emoji} ${pot.name} cần tưới nước',
            body:
                'Độ ẩm đất hiện tại ${pot.moisture.toStringAsFixed(0)}% — dưới mức tối ưu.',
            time: DateTime.now().subtract(const Duration(minutes: 15)),
            isRead: false,
          ),
        );
      }
      if (pot.status == MoistureStatus.critical) {
        _notifications.add(
          _Notification(
            type: _NotifType.critical,
            title: '⚠️ ${pot.name} — nguy hiểm!',
            body:
                'Độ ẩm xuống quá thấp. Cây có thể bị héo nếu không tưới ngay.',
            time: DateTime.now().subtract(const Duration(minutes: 5)),
            isRead: false,
          ),
        );
      }
      if (pot.autoWaterEnabled && pot.status == MoistureStatus.wet) {
        _notifications.add(
          _Notification(
            type: _NotifType.info,
            title: '💧 ${pot.name} đã được tưới tự động',
            body: 'Hệ thống tự động tưới lúc ${_formatTime(pot.lastWatered)}.',
            time: pot.lastWatered,
            isRead: true,
          ),
        );
      }
    }

    // Sort: unread first, then by time
    _notifications.sort((a, b) {
      if (a.isRead != b.isRead) return a.isRead ? 1 : -1;
      return b.time.compareTo(a.time);
    });

    // Add a system notification
    _notifications.add(
      _Notification(
        type: _NotifType.system,
        title: '🌱 PlantSense đã kết nối',
        body: 'Hệ thống đang hoạt động bình thường. Cảm biến đang ghi dữ liệu.',
        time: DateTime.now().subtract(const Duration(hours: 1)),
        isRead: true,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final unreadCount = _notifications.where((n) => !n.isRead).length;
    final hasNeedsWater = widget.pots.any(
      (p) =>
          p.status == MoistureStatus.dry || p.status == MoistureStatus.critical,
    );

    return Scaffold(
      backgroundColor: Theme.of(context).colorScheme.surfaceContainerLowest,
      appBar: AppBar(
        backgroundColor: AppColors.darkGreen,
        foregroundColor: Colors.white,
        title: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Thông báo',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
            ),
            if (unreadCount > 0)
              Text(
                '$unreadCount chưa đọc',
                style: const TextStyle(fontSize: 11, color: Colors.white70),
              ),
          ],
        ),
        actions: [
          if (hasNeedsWater)
            TextButton.icon(
              onPressed: () => Navigator.pop(context, true),
              icon: const Icon(
                Icons.water_drop_outlined,
                size: 16,
                color: Colors.white70,
              ),
              label: const Text(
                'Xem chậu cần tưới',
                style: TextStyle(color: Colors.white70, fontSize: 12),
              ),
              style: TextButton.styleFrom(
                padding: const EdgeInsets.symmetric(horizontal: 8),
              ),
            ),
          if (unreadCount > 0)
            TextButton(
              onPressed: () {
                setState(() {
                  for (final n in _notifications) {
                    n.isRead = true;
                  }
                });
              },
              child: const Text(
                'Đọc tất cả',
                style: TextStyle(color: Colors.white70, fontSize: 13),
              ),
            ),
        ],
      ),
      body: _notifications.isEmpty
          ? const _EmptyNotifications()
          : ListView.separated(
              padding: const EdgeInsets.symmetric(vertical: 12),
              itemCount: _notifications.length,
              separatorBuilder: (_, __) => const SizedBox(height: 1),
              itemBuilder: (context, index) {
                final notif = _notifications[index];
                return _NotificationTile(
                  notification: notif,
                  onTap: () => setState(() => notif.isRead = true),
                );
              },
            ),
    );
  }

  String _formatTime(DateTime dt) {
    final diff = DateTime.now().difference(dt);
    if (diff.inMinutes < 60) return '${diff.inMinutes} phút trước';
    if (diff.inHours < 24) return '${diff.inHours} giờ trước';
    return '${diff.inDays} ngày trước';
  }
}

// ─────────────────────────────────────────
// Notification model
// ─────────────────────────────────────────

enum _NotifType { warning, critical, info, system }

class _Notification {
  final _NotifType type;
  final String title;
  final String body;
  final DateTime time;
  bool isRead;

  _Notification({
    required this.type,
    required this.title,
    required this.body,
    required this.time,
    required this.isRead,
  });
}

extension on _NotifType {
  Color get color {
    switch (this) {
      case _NotifType.warning:
        return AppColors.orange;
      case _NotifType.critical:
        return AppColors.red;
      case _NotifType.info:
        return AppColors.blue;
      case _NotifType.system:
        return AppColors.green;
    }
  }

  IconData get icon {
    switch (this) {
      case _NotifType.warning:
        return Icons.water_drop_outlined;
      case _NotifType.critical:
        return Icons.warning_amber_rounded;
      case _NotifType.info:
        return Icons.check_circle_outline;
      case _NotifType.system:
        return Icons.sensors;
    }
  }
}

// ─────────────────────────────────────────
// Notification Tile
// ─────────────────────────────────────────

class _NotificationTile extends StatelessWidget {
  final _Notification notification;
  final VoidCallback onTap;

  const _NotificationTile({required this.notification, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final n = notification;
    return InkWell(
      onTap: onTap,
      child: Container(
        color: n.isRead
            ? Colors.transparent
            : n.type.color.withValues(alpha: 0.04),
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Container(
              width: 42,
              height: 42,
              decoration: BoxDecoration(
                color: n.type.color.withValues(alpha: 0.12),
                shape: BoxShape.circle,
              ),
              child: Icon(n.type.icon, size: 20, color: n.type.color),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Expanded(
                        child: Text(
                          n.title,
                          style: TextStyle(
                            fontWeight: n.isRead
                                ? FontWeight.normal
                                : FontWeight.bold,
                            fontSize: 14,
                          ),
                        ),
                      ),
                      if (!n.isRead)
                        Container(
                          width: 8,
                          height: 8,
                          decoration: BoxDecoration(
                            color: n.type.color,
                            shape: BoxShape.circle,
                          ),
                        ),
                    ],
                  ),
                  const SizedBox(height: 4),
                  Text(
                    n.body,
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.grey.shade600,
                      height: 1.4,
                    ),
                  ),
                  const SizedBox(height: 6),
                  Text(
                    _timeAgo(n.time),
                    style: TextStyle(fontSize: 11, color: Colors.grey.shade400),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  String _timeAgo(DateTime dt) {
    final diff = DateTime.now().difference(dt);
    if (diff.inMinutes < 1) return 'Vừa xong';
    if (diff.inMinutes < 60) return '${diff.inMinutes} phút trước';
    if (diff.inHours < 24) return '${diff.inHours} giờ trước';
    return '${diff.inDays} ngày trước';
  }
}

class _EmptyNotifications extends StatelessWidget {
  const _EmptyNotifications();

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Container(
            width: 80,
            height: 80,
            decoration: BoxDecoration(
              color: AppColors.green.withValues(alpha: 0.1),
              shape: BoxShape.circle,
            ),
            child: const Icon(
              Icons.notifications_none_outlined,
              size: 40,
              color: AppColors.green,
            ),
          ),
          const SizedBox(height: 16),
          const Text(
            'Không có thông báo',
            style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 8),
          Text(
            'Mọi cây đều đang khỏe mạnh!',
            style: TextStyle(fontSize: 13, color: Colors.grey.shade500),
          ),
        ],
      ),
    );
  }
}
