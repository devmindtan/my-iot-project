import 'package:flutter/material.dart';

import '../theme/app_colors.dart';

class FutureFeaturesBanner extends StatelessWidget {
  const FutureFeaturesBanner({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            AppColors.darkGreen.withValues(alpha: 0.08),
            AppColors.green.withValues(alpha: 0.05),
          ],
        ),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: AppColors.green.withValues(alpha: 0.2)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Row(
            children: [
              Icon(
                Icons.rocket_launch_outlined,
                color: AppColors.green,
                size: 18,
              ),
              SizedBox(width: 8),
              Text(
                'Tính năng sắp ra mắt',
                style: TextStyle(
                  fontWeight: FontWeight.bold,
                  color: AppColors.green,
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          _FeatureItem(
            icon: Icons.videocam_outlined,
            title: 'Camera theo dõi 24/7',
            description: 'Xem trực tiếp + AI nhận diện bệnh cây',
          ),
          _FeatureItem(
            icon: Icons.psychology_outlined,
            title: 'AI chẩn đoán cây',
            description: 'Phân tích ảnh và đưa lời khuyên tức thì',
          ),
          _FeatureItem(
            icon: Icons.notifications_active_outlined,
            title: 'Cảnh báo thông minh',
            description: 'Thông báo dựa trên thời tiết và mùa vụ',
          ),
          _FeatureItem(
            icon: Icons.hub_outlined,
            title: 'Kết nối IoT đa chậu',
            description: 'Quản lý lên đến 50 chậu cùng lúc',
            isLast: true,
          ),
        ],
      ),
    );
  }
}

class _FeatureItem extends StatelessWidget {
  final IconData icon;
  final String title;
  final String description;
  final bool isLast;

  const _FeatureItem({
    required this.icon,
    required this.title,
    required this.description,
    this.isLast = false,
  });

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: EdgeInsets.only(bottom: isLast ? 0 : 10),
      child: Row(
        children: [
          Container(
            width: 36,
            height: 36,
            decoration: BoxDecoration(
              color: AppColors.green.withValues(alpha: 0.1),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Icon(icon, size: 18, color: AppColors.green),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: const TextStyle(
                    fontSize: 13,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                Text(
                  description,
                  style: TextStyle(fontSize: 11, color: Colors.grey.shade600),
                ),
              ],
            ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
            decoration: BoxDecoration(
              color: Colors.amber.shade100,
              borderRadius: BorderRadius.circular(6),
            ),
            child: Text(
              'Sắp có',
              style: TextStyle(
                fontSize: 10,
                color: Colors.amber.shade800,
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
