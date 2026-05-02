import 'package:flutter/material.dart';

import '../data/backend_api.dart';
import '../models/plant_pot.dart';
import '../theme/app_colors.dart';

class NotificationsScreen extends StatefulWidget {
  final List<PlantPot> pots;
  const NotificationsScreen({super.key, required this.pots});

  @override
  State<NotificationsScreen> createState() => _NotificationsScreenState();
}

class _NotificationsScreenState extends State<NotificationsScreen> {
  List<ApiNotificationItem> _notifications = const [];
  List<ApiWateringLogItem> _wateringLogs = const [];
  bool _loading = true;
  String? _error;

  @override
  void initState() {
    super.initState();
    _loadData();
  }

  Future<void> _loadData() async {
    setState(() {
      _loading = true;
      _error = null;
    });

    try {
      final notifications = await BackendApi.fetchNotifications(limit: 100);
      final logs = await BackendApi.fetchWateringLogs(limit: 20);

      if (!mounted) return;
      setState(() {
        _notifications = notifications;
        _wateringLogs = logs;
        _loading = false;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _error = e.toString().replaceFirst('Exception: ', '');
        _loading = false;
      });
    }
  }

  Future<void> _markAllRead() async {
    try {
      await BackendApi.markAllNotificationsRead();
      if (!mounted) return;
      setState(() {
        for (final n in _notifications) {
          n.isRead = true;
        }
      });
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Khong the danh dau da doc: $e')));
    }
  }

  Future<void> _markOneRead(ApiNotificationItem notification) async {
    if (notification.isRead) return;

    setState(() => notification.isRead = true);
    try {
      await BackendApi.markNotificationRead(notification.id);
    } catch (e) {
      if (!mounted) return;
      setState(() => notification.isRead = false);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Khong the cap nhat thong bao: $e')),
      );
    }
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
              'Thong bao',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
            ),
            if (unreadCount > 0)
              Text(
                '$unreadCount chua doc',
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
                'Xem chau can tuoi',
                style: TextStyle(color: Colors.white70, fontSize: 12),
              ),
              style: TextButton.styleFrom(
                padding: const EdgeInsets.symmetric(horizontal: 8),
              ),
            ),
          if (unreadCount > 0)
            TextButton(
              onPressed: _markAllRead,
              child: const Text(
                'Doc tat ca',
                style: TextStyle(color: Colors.white70, fontSize: 13),
              ),
            ),
        ],
      ),
      body: RefreshIndicator(onRefresh: _loadData, child: _buildBody()),
    );
  }

  Widget _buildBody() {
    if (_loading) {
      return const Center(child: CircularProgressIndicator());
    }

    if (_error != null) {
      return ListView(
        children: [
          const SizedBox(height: 80),
          _ErrorState(message: _error!, onRetry: _loadData),
        ],
      );
    }

    if (_notifications.isEmpty && _wateringLogs.isEmpty) {
      return ListView(children: [SizedBox(height: 80), _EmptyNotifications()]);
    }

    return ListView(
      padding: const EdgeInsets.symmetric(vertical: 12),
      children: [
        if (_notifications.isNotEmpty) ...[
          const _SectionTitle(
            icon: Icons.notifications_outlined,
            title: 'Thong bao he thong',
          ),
          ..._notifications.map(
            (notif) => _NotificationTile(
              notification: notif,
              onTap: () => _markOneRead(notif),
            ),
          ),
          const SizedBox(height: 10),
        ],
        if (_wateringLogs.isNotEmpty) ...[
          const _SectionTitle(
            icon: Icons.water_drop_outlined,
            title: 'Lich su tuoi gan day',
          ),
          ..._wateringLogs.map((log) => _WateringLogTile(log: log)),
        ],
      ],
    );
  }
}

class _SectionTitle extends StatelessWidget {
  final IconData icon;
  final String title;

  const _SectionTitle({required this.icon, required this.title});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 8, 16, 8),
      child: Row(
        children: [
          Icon(icon, size: 16, color: AppColors.green),
          const SizedBox(width: 8),
          Text(
            title,
            style: const TextStyle(
              fontSize: 13,
              fontWeight: FontWeight.w700,
              color: AppColors.green,
            ),
          ),
        ],
      ),
    );
  }
}

class _NotificationTile extends StatelessWidget {
  final ApiNotificationItem notification;
  final VoidCallback onTap;

  const _NotificationTile({required this.notification, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final n = notification;
    final style = _notifStyle(n.type);

    return InkWell(
      onTap: onTap,
      child: Container(
        color: n.isRead
            ? Colors.transparent
            : style.color.withValues(alpha: 0.05),
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Container(
              width: 42,
              height: 42,
              decoration: BoxDecoration(
                color: style.color.withValues(alpha: 0.12),
                shape: BoxShape.circle,
              ),
              child: Icon(style.icon, size: 20, color: style.color),
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
                            color: style.color,
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
                    _timeAgo(n.createdAt),
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
}

class _WateringLogTile extends StatelessWidget {
  final ApiWateringLogItem log;

  const _WateringLogTile({required this.log});

  @override
  Widget build(BuildContext context) {
    final label = switch (log.triggeredBy) {
      'auto' => 'Tu dong',
      'scheduled' => 'Theo lich',
      _ => 'Thu cong',
    };

    final before = log.moistureBefore;
    final after = log.moistureAfter;
    final delta = before != null && after != null ? after - before : null;

    return Container(
      margin: const EdgeInsets.fromLTRB(12, 0, 12, 8),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: Colors.blue.withValues(alpha: 0.25)),
      ),
      child: Row(
        children: [
          Container(
            width: 38,
            height: 38,
            decoration: BoxDecoration(
              color: AppColors.blue.withValues(alpha: 0.12),
              borderRadius: BorderRadius.circular(10),
            ),
            child: const Icon(
              Icons.water_drop,
              color: AppColors.blue,
              size: 20,
            ),
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  log.potName ?? 'Khong ro chau',
                  style: const TextStyle(fontWeight: FontWeight.w600),
                ),
                const SizedBox(height: 2),
                Text(
                  '$label · ${log.durationSeconds ?? 0}s · ${_timeAgo(log.createdAt)}',
                  style: TextStyle(fontSize: 12, color: Colors.grey.shade600),
                ),
              ],
            ),
          ),
          if (delta != null)
            Text(
              '+${delta.toStringAsFixed(1)}%',
              style: const TextStyle(
                color: AppColors.blue,
                fontWeight: FontWeight.w700,
                fontSize: 12,
              ),
            ),
        ],
      ),
    );
  }
}

class _ErrorState extends StatelessWidget {
  final String message;
  final Future<void> Function() onRetry;

  const _ErrorState({required this.message, required this.onRetry});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 24),
      child: Column(
        children: [
          const Icon(Icons.error_outline, size: 42, color: AppColors.red),
          const SizedBox(height: 10),
          const Text(
            'Khong tai duoc du lieu',
            style: TextStyle(fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 6),
          Text(
            message,
            textAlign: TextAlign.center,
            style: TextStyle(fontSize: 12, color: Colors.grey.shade600),
          ),
          const SizedBox(height: 14),
          OutlinedButton.icon(
            onPressed: () => onRetry(),
            icon: const Icon(Icons.refresh),
            label: const Text('Thu lai'),
          ),
        ],
      ),
    );
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
            'Khong co thong bao',
            style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 8),
          Text(
            'He thong dang hoat dong binh thuong!',
            style: TextStyle(fontSize: 13, color: Colors.grey.shade500),
          ),
        ],
      ),
    );
  }
}

class _NotifStyle {
  final Color color;
  final IconData icon;

  const _NotifStyle({required this.color, required this.icon});
}

_NotifStyle _notifStyle(String type) {
  switch (type) {
    case 'dry':
      return const _NotifStyle(
        color: AppColors.orange,
        icon: Icons.water_drop_outlined,
      );
    case 'critical':
      return const _NotifStyle(
        color: AppColors.red,
        icon: Icons.warning_amber_rounded,
      );
    case 'system':
      return const _NotifStyle(color: AppColors.green, icon: Icons.sensors);
    case 'wet':
      return const _NotifStyle(
        color: AppColors.blue,
        icon: Icons.check_circle_outline,
      );
    case 'info':
    default:
      return const _NotifStyle(color: AppColors.blue, icon: Icons.info_outline);
  }
}

String _timeAgo(DateTime dt) {
  final diff = DateTime.now().difference(dt);
  if (diff.inMinutes < 1) return 'Vua xong';
  if (diff.inMinutes < 60) return '${diff.inMinutes} phut truoc';
  if (diff.inHours < 24) return '${diff.inHours} gio truoc';
  return '${diff.inDays} ngay truoc';
}
