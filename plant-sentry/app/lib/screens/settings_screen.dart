import 'package:flutter/material.dart';

import '../theme/app_colors.dart';

class SettingsScreen extends StatefulWidget {
  final ThemeMode themeMode;
  final ValueChanged<ThemeMode> onThemeModeChanged;

  const SettingsScreen({
    super.key,
    required this.themeMode,
    required this.onThemeModeChanged,
  });

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  bool _pushNotifications = true;
  bool _autoWaterGlobal = false;
  double _alertThreshold = 40;
  String _unit = 'Celsius';

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Theme.of(context).colorScheme.surfaceContainerLowest,
      appBar: AppBar(
        backgroundColor: AppColors.darkGreen,
        foregroundColor: Colors.white,
        title: const Text(
          'Cài đặt',
          style: TextStyle(fontWeight: FontWeight.bold),
        ),
      ),
      body: ListView(
        padding: const EdgeInsets.symmetric(vertical: 12),
        children: [
          // ── Section: Notifications ──
          _SectionHeader(
            icon: Icons.notifications_outlined,
            title: 'Thông báo',
          ),
          _SettingsCard(
            children: [
              _SwitchTile(
                icon: Icons.notifications_active_outlined,
                iconColor: AppColors.orange,
                title: 'Thông báo đẩy',
                subtitle: 'Nhận cảnh báo khi cây cần chăm sóc',
                value: _pushNotifications,
                onChanged: (v) => setState(() => _pushNotifications = v),
              ),
              _Divider(),
              _SwitchTile(
                icon: Icons.volume_up_outlined,
                iconColor: Colors.blue.shade600,
                title: 'Âm thanh cảnh báo',
                subtitle: 'Phát âm khi có thông báo mới',
                value: true,
                onChanged: (_) {},
              ),
            ],
          ),

          // ── Section: Auto Water ──
          _SectionHeader(
            icon: Icons.water_drop_outlined,
            title: 'Tưới tự động',
          ),
          _SettingsCard(
            children: [
              _SwitchTile(
                icon: Icons.autorenew,
                iconColor: AppColors.green,
                title: 'Bật tưới toàn cục',
                subtitle: 'Áp dụng cho tất cả chậu đã bật tưới tự động',
                value: _autoWaterGlobal,
                onChanged: (v) => setState(() => _autoWaterGlobal = v),
              ),
              _Divider(),
              Padding(
                padding: const EdgeInsets.fromLTRB(16, 12, 16, 4),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        Icon(
                          Icons.water_drop,
                          size: 18,
                          color: Colors.blue.shade600,
                        ),
                        const SizedBox(width: 10),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              const Text(
                                'Ngưỡng tưới',
                                style: TextStyle(fontWeight: FontWeight.w500),
                              ),
                              Text(
                                'Tưới khi độ ẩm < ${_alertThreshold.toStringAsFixed(0)}%',
                                style: TextStyle(
                                  fontSize: 12,
                                  color: Colors.grey.shade600,
                                ),
                              ),
                            ],
                          ),
                        ),
                        Container(
                          padding: const EdgeInsets.symmetric(
                            horizontal: 10,
                            vertical: 4,
                          ),
                          decoration: BoxDecoration(
                            color: AppColors.blue.withValues(alpha: 0.1),
                            borderRadius: BorderRadius.circular(8),
                          ),
                          child: Text(
                            '${_alertThreshold.toStringAsFixed(0)}%',
                            style: const TextStyle(
                              color: AppColors.blue,
                              fontWeight: FontWeight.bold,
                            ),
                          ),
                        ),
                      ],
                    ),
                    Slider(
                      value: _alertThreshold,
                      min: 10,
                      max: 60,
                      divisions: 10,
                      activeColor: AppColors.green,
                      onChanged: (v) => setState(() => _alertThreshold = v),
                    ),
                  ],
                ),
              ),
            ],
          ),

          // ── Section: Display ──
          _SectionHeader(icon: Icons.palette_outlined, title: 'Hiển thị'),
          _SettingsCard(
            children: [
              _SelectTile(
                icon: Icons.dark_mode_outlined,
                iconColor: Colors.indigo.shade400,
                title: 'Chế độ giao diện',
                subtitle: 'System / Light / Dark',
                value: _themeModeLabel(widget.themeMode),
                options: const ['System', 'Light', 'Dark'],
                onChanged: (modeLabel) {
                  final mode = _themeModeFromLabel(modeLabel);
                  widget.onThemeModeChanged(mode);
                  setState(() {});
                },
              ),
              _Divider(),
              _SelectTile(
                icon: Icons.thermostat_outlined,
                iconColor: Colors.orange.shade600,
                title: 'Đơn vị nhiệt độ',
                subtitle: '',
                value: _unit,
                options: const ['Celsius', 'Fahrenheit'],
                onChanged: (v) => setState(() => _unit = v),
              ),
            ],
          ),

          // ── Section: Device ──
          _SectionHeader(icon: Icons.sensors, title: 'Thiết bị IoT'),
          _SettingsCard(
            children: [
              _NavTile(
                icon: Icons.wifi_outlined,
                iconColor: Colors.green.shade600,
                title: 'Kết nối WiFi',
                subtitle: 'Quản lý mạng kết nối cảm biến',
                onTap: () => _showComingSoon(context),
              ),
              _Divider(),
              _NavTile(
                icon: Icons.bluetooth_outlined,
                iconColor: Colors.blue.shade600,
                title: 'Ghép cặp thiết bị',
                subtitle: 'Thêm cảm biến hoặc van tưới mới',
                onTap: () => _showComingSoon(context),
              ),
              _Divider(),
              _NavTile(
                icon: Icons.update_outlined,
                iconColor: Colors.teal.shade600,
                title: 'Cập nhật firmware',
                subtitle: 'Phiên bản firmware: v1.2.0',
                onTap: () => _showComingSoon(context),
              ),
            ],
          ),

          // ── Section: About ──
          _SectionHeader(icon: Icons.info_outlined, title: 'Về ứng dụng'),
          _SettingsCard(
            children: [
              _NavTile(
                icon: Icons.phone_android_outlined,
                iconColor: AppColors.green,
                title: 'PlantSense',
                subtitle: 'Phiên bản 1.0.0',
                onTap: () {},
                trailing: Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 8,
                    vertical: 3,
                  ),
                  decoration: BoxDecoration(
                    color: Colors.green.shade100,
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Text(
                    'Mới nhất',
                    style: TextStyle(
                      color: Colors.green.shade700,
                      fontSize: 11,
                      fontWeight: FontWeight.w500,
                    ),
                  ),
                ),
              ),
              _Divider(),
              _NavTile(
                icon: Icons.privacy_tip_outlined,
                iconColor: Colors.grey.shade600,
                title: 'Chính sách bảo mật',
                subtitle: '',
                onTap: () {},
              ),
            ],
          ),

          const SizedBox(height: 32),
        ],
      ),
    );
  }

  void _showComingSoon(BuildContext context) {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('🚀 Tính năng này sắp ra mắt!'),
        backgroundColor: AppColors.green,
      ),
    );
  }

  String _themeModeLabel(ThemeMode mode) {
    switch (mode) {
      case ThemeMode.light:
        return 'Light';
      case ThemeMode.dark:
        return 'Dark';
      case ThemeMode.system:
        return 'System';
    }
  }

  ThemeMode _themeModeFromLabel(String label) {
    switch (label) {
      case 'Light':
        return ThemeMode.light;
      case 'Dark':
        return ThemeMode.dark;
      case 'System':
      default:
        return ThemeMode.system;
    }
  }
}

// ─────────────────────────────────────────
// Settings UI Components
// ─────────────────────────────────────────

class _SectionHeader extends StatelessWidget {
  final IconData icon;
  final String title;

  const _SectionHeader({required this.icon, required this.title});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 20, 16, 6),
      child: Row(
        children: [
          Icon(icon, size: 15, color: AppColors.green),
          const SizedBox(width: 6),
          Text(
            title.toUpperCase(),
            style: TextStyle(
              fontSize: 11,
              fontWeight: FontWeight.w700,
              color: AppColors.green,
              letterSpacing: 0.8,
            ),
          ),
        ],
      ),
    );
  }
}

class _SettingsCard extends StatelessWidget {
  final List<Widget> children;
  const _SettingsCard({required this.children});

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: Colors.grey.withValues(alpha: 0.12)),
      ),
      child: Column(children: children),
    );
  }
}

class _Divider extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Divider(
      height: 1,
      indent: 56,
      color: Colors.grey.withValues(alpha: 0.15),
    );
  }
}

class _SelectTile extends StatelessWidget {
  final IconData icon;
  final Color iconColor;
  final String title;
  final String subtitle;
  final String value;
  final List<String> options;
  final ValueChanged<String> onChanged;

  const _SelectTile({
    required this.icon,
    required this.iconColor,
    required this.title,
    required this.subtitle,
    required this.value,
    required this.options,
    required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      child: Row(
        children: [
          _IconBox(icon: icon, color: iconColor),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(fontWeight: FontWeight.w500),
                ),
                if (subtitle.isNotEmpty)
                  Text(
                    subtitle,
                    style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
                  ),
              ],
            ),
          ),
          DropdownButton<String>(
            value: value,
            underline: const SizedBox(),
            style: TextStyle(
              fontSize: 13,
              color: Theme.of(context).colorScheme.onSurface,
            ),
            items: options
                .map((o) => DropdownMenuItem(value: o, child: Text(o)))
                .toList(),
            onChanged: (v) {
              if (v != null) onChanged(v);
            },
          ),
        ],
      ),
    );
  }
}

class _NavTile extends StatelessWidget {
  final IconData icon;
  final Color iconColor;
  final String title;
  final String subtitle;
  final VoidCallback onTap;
  final Widget? trailing;

  const _NavTile({
    required this.icon,
    required this.iconColor,
    required this.title,
    required this.subtitle,
    required this.onTap,
    this.trailing,
  });

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(14),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        child: Row(
          children: [
            _IconBox(icon: icon, color: iconColor),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    style: const TextStyle(fontWeight: FontWeight.w500),
                  ),
                  if (subtitle.isNotEmpty)
                    Text(
                      subtitle,
                      style: TextStyle(
                        fontSize: 12,
                        color: Colors.grey.shade500,
                      ),
                    ),
                ],
              ),
            ),
            trailing ??
                Icon(
                  Icons.chevron_right,
                  color: Colors.grey.shade400,
                  size: 20,
                ),
          ],
        ),
      ),
    );
  }
}

class _SwitchTile extends StatelessWidget {
  final IconData icon;
  final Color iconColor;
  final String title;
  final bool value;
  final ValueChanged<bool> onChanged;

  const _SwitchTile({
    required this.icon,
    required this.iconColor,
    required this.title,
    required this.subtitle,
    required this.value,
    required this.onChanged,
  });

  final String subtitle;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      child: Row(
        children: [
          _IconBox(icon: icon, color: iconColor),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(fontWeight: FontWeight.w500),
                ),
                if (subtitle.isNotEmpty)
                  Text(
                    subtitle,
                    style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
                  ),
              ],
            ),
          ),
          Switch(
            value: value,
            onChanged: onChanged,
            activeColor: AppColors.green,
          ),
        ],
      ),
    );
  }
}

class _IconBox extends StatelessWidget {
  final IconData icon;
  final Color color;

  const _IconBox({required this.icon, required this.color});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 36,
      height: 36,
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(9),
      ),
      child: Icon(icon, size: 18, color: color),
    );
  }
}
