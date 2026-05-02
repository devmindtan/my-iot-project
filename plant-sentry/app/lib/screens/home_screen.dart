import 'package:flutter/material.dart';

import '../data/backend_api.dart';
import '../models/plant_pot.dart';
import '../theme/app_colors.dart';
import 'dashboard_screen.dart';
import 'notifications_screen.dart';
import 'settings_screen.dart';

class HomeScreen extends StatefulWidget {
  final ThemeMode themeMode;
  final ValueChanged<ThemeMode> onThemeModeChanged;
  final List<PlantPot> initialPots;

  const HomeScreen({
    super.key,
    required this.themeMode,
    required this.onThemeModeChanged,
    required this.initialPots,
  });

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  int _selectedIndex = 0;
  List<PlantPot> _pots = const [];

  @override
  void initState() {
    super.initState();
    _pots = List<PlantPot>.from(widget.initialPots);
  }

  Future<void> _refreshPots() async {
    try {
      final apiPots = await BackendApi.fetchDashboard();
      if (!mounted) return;
      setState(() {
        _pots = apiPots.map((pot) => pot.toPlantPot()).toList();
      });
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text('Khong the tai dashboard: $e')));
    }
  }

  void _onNavTap(int index) {
    setState(() => _selectedIndex = index);
  }

  Widget _buildComingSoon(String title, IconData icon, String description) {
    return Scaffold(
      appBar: AppBar(
        title: Text(title),
        backgroundColor: AppColors.darkGreen,
        foregroundColor: Colors.white,
        elevation: 0,
      ),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(32),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Container(
                width: 100,
                height: 100,
                decoration: BoxDecoration(
                  color: AppColors.green.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(24),
                ),
                child: Icon(icon, size: 48, color: AppColors.green),
              ),
              const SizedBox(height: 24),
              Text(
                title,
                style: const TextStyle(
                  fontSize: 22,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 12),
              Text(
                description,
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontSize: 15,
                  color: Colors.grey.shade600,
                  height: 1.5,
                ),
              ),
              const SizedBox(height: 32),
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 20,
                  vertical: 10,
                ),
                decoration: BoxDecoration(
                  color: AppColors.green.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(
                    color: AppColors.green.withValues(alpha: 0.3),
                  ),
                ),
                child: const Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.schedule, size: 16, color: AppColors.green),
                    SizedBox(width: 8),
                    Text(
                      'Sắp ra mắt',
                      style: TextStyle(
                        color: AppColors.green,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _getScreen() {
    switch (_selectedIndex) {
      case 0:
        return DashboardScreen(
          pots: _pots,
          onPotAdded: (newPot) => setState(() => _pots = [..._pots, newPot]),
          onRefresh: _refreshPots,
        );
      case 1:
        return _buildComingSoon(
          'Camera Theo Dõi',
          Icons.videocam_outlined,
          'Xem trực tiếp từ camera tích hợp để quan sát cây trồng mọi lúc mọi nơi. Hỗ trợ AI nhận diện tình trạng sức khỏe cây.',
        );
      case 2:
        return _buildComingSoon(
          'Tự Động Tưới',
          Icons.settings_remote_outlined,
          'Kết nối với hệ thống van tưới tự động. Lập lịch và điều khiển từ xa qua ứng dụng.',
        );
      case 3:
        return _buildComingSoon(
          'Phân Tích AI',
          Icons.psychology_outlined,
          'Hệ thống AI phân tích dữ liệu cảm biến và đưa ra lời khuyên chăm sóc cây cá nhân hóa.',
        );
      default:
        return DashboardScreen(pots: _pots);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Navigator(
      onGenerateRoute: (settings) {
        if (settings.name == '/notifications') {
          return MaterialPageRoute(
            builder: (_) => NotificationsScreen(pots: _pots),
          );
        }
        if (settings.name == '/settings') {
          return MaterialPageRoute(
            builder: (_) => SettingsScreen(
              themeMode: widget.themeMode,
              onThemeModeChanged: widget.onThemeModeChanged,
            ),
          );
        }
        return MaterialPageRoute(
          builder: (_) => Scaffold(
            body: _getScreen(),
            bottomNavigationBar: NavigationBar(
              selectedIndex: _selectedIndex,
              onDestinationSelected: _onNavTap,
              destinations: const [
                NavigationDestination(
                  icon: Icon(Icons.dashboard_outlined),
                  selectedIcon: Icon(Icons.dashboard),
                  label: 'Tổng quan',
                ),
                NavigationDestination(
                  icon: _ComingSoonBadge(child: Icon(Icons.videocam_outlined)),
                  selectedIcon: _ComingSoonBadge(child: Icon(Icons.videocam)),
                  label: 'Camera',
                ),
                NavigationDestination(
                  icon: _ComingSoonBadge(
                    child: Icon(Icons.water_drop_outlined),
                  ),
                  selectedIcon: _ComingSoonBadge(child: Icon(Icons.water_drop)),
                  label: 'Tưới tự động',
                ),
                NavigationDestination(
                  icon: _ComingSoonBadge(
                    child: Icon(Icons.psychology_outlined),
                  ),
                  selectedIcon: _ComingSoonBadge(child: Icon(Icons.psychology)),
                  label: 'AI Phân tích',
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}

class _ComingSoonBadge extends StatelessWidget {
  final Widget child;
  const _ComingSoonBadge({required this.child});

  @override
  Widget build(BuildContext context) {
    return Stack(
      clipBehavior: Clip.none,
      children: [
        child,
        Positioned(
          top: -4,
          right: -4,
          child: Container(
            width: 8,
            height: 8,
            decoration: const BoxDecoration(
              color: AppColors.orange,
              shape: BoxShape.circle,
            ),
          ),
        ),
      ],
    );
  }
}
