import 'dart:developer' as dev;

import 'package:flutter/material.dart';

import '../data/backend_api.dart';
import '../models/plant_pot.dart';
import 'home_screen.dart';

class StartupLoadingScreen extends StatefulWidget {
  final ThemeMode themeMode;
  final ValueChanged<ThemeMode> onThemeModeChanged;

  const StartupLoadingScreen({
    super.key,
    required this.themeMode,
    required this.onThemeModeChanged,
  });

  @override
  State<StartupLoadingScreen> createState() => _StartupLoadingScreenState();
}

class _StartupLoadingScreenState extends State<StartupLoadingScreen> {
  double _progress = 0.0;
  String _status = 'Khởi tạo...';
  String? _error;

  @override
  void initState() {
    super.initState();
    _bootstrap();
  }

  Future<void> _bootstrap() async {
    setState(() {
      _error = null;
      _progress = 0.08;
      _status = 'Khởi tạo kết nối server...';
    });

    try {
      setState(() {
        _progress = 0.35;
        _status = 'Đang tải dữ liệu về...';
      });

      final apiPots = await BackendApi.fetchDashboard();

      setState(() {
        _progress = 0.72;
        _status = 'Đang xử lí dữ liệu...';
      });

      final pots = apiPots.map((pot) => pot.toPlantPot()).toList();

      setState(() {
        _progress = 0.96;
        _status = 'Hoàn tất tải dữ liệu...';
      });

      await Future.delayed(const Duration(milliseconds: 220));
      if (!mounted) return;

      setState(() {
        _progress = 1.0;
        _status = 'Sẵn sàng';
      });

      await Future.delayed(const Duration(milliseconds: 120));
      if (!mounted) return;

      Navigator.of(context).pushReplacement(
        MaterialPageRoute(
          builder: (_) => HomeScreen(
            themeMode: widget.themeMode,
            onThemeModeChanged: widget.onThemeModeChanged,
            initialPots: pots,
          ),
        ),
      );
    } catch (e, st) {
      dev.log(
        '[StartupLoading] Error loading startup data: $e',
        name: 'StartupLoading',
        error: e,
        stackTrace: st,
      );
      if (!mounted) return;
      setState(() {
        _error = e.toString().replaceFirst('Exception: ', '');
        _status = 'Không thể tải dữ liệu';
        _progress = 0.0;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_error != null) {
      return Scaffold(
        backgroundColor: const Color(0xFFE7F4EC),
        body: Center(
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Icon(Icons.cloud_off, size: 62, color: Color(0xFF3D6154)),
                const SizedBox(height: 16),
                const Text(
                  'Không thể kết nối server',
                  style: TextStyle(fontWeight: FontWeight.w700, fontSize: 18),
                ),
                const SizedBox(height: 8),
                Text(
                  _error!,
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    fontSize: 13,
                    color: Color(0xFF4B6D61),
                  ),
                ),
                const SizedBox(height: 22),
                FilledButton.icon(
                  onPressed: _bootstrap,
                  icon: const Icon(Icons.refresh),
                  label: const Text('Thu lai'),
                ),
              ],
            ),
          ),
        ),
      );
    }

    return Scaffold(
      backgroundColor: const Color(0xFFE7F4EC),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 24),
          child: ConstrainedBox(
            constraints: const BoxConstraints(maxWidth: 320),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Image.asset('assets/branding/loading_logo.png', width: 210),
                const SizedBox(height: 24),
                Text(
                  _status,
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    color: Color(0xFF134E3A),
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const SizedBox(height: 14),
                Container(
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(999),
                    border: Border.all(
                      color: const Color(0xFF0F6B45).withValues(alpha: 0.35),
                    ),
                  ),
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(999),
                    child: LinearProgressIndicator(
                      value: _progress.clamp(0.0, 1.0),
                      minHeight: 10,
                      backgroundColor: const Color(0xFFCFE6DA),
                      color: const Color(0xFF0F6B45),
                    ),
                  ),
                ),
                const SizedBox(height: 10),
                Text(
                  '${(_progress * 100).clamp(0, 100).toStringAsFixed(0)}% - Kết nối server và tải dữ liệu',
                  textAlign: TextAlign.center,
                  style: const TextStyle(
                    fontSize: 12,
                    color: Color(0xFF3C6759),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
