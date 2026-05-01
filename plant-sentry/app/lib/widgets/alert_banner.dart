import 'package:flutter/material.dart';

import '../theme/app_colors.dart';

class AlertBanner extends StatelessWidget {
  final int count;
  final VoidCallback? onViewTap;

  const AlertBanner({super.key, required this.count, this.onViewTap});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: AppColors.orange.withValues(alpha: 0.12),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: AppColors.orange.withValues(alpha: 0.4)),
      ),
      child: Row(
        children: [
          const Icon(
            Icons.warning_amber_rounded,
            color: AppColors.orange,
            size: 20,
          ),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              '$count chậu cần tưới nước ngay!',
              style: const TextStyle(
                color: AppColors.orange,
                fontWeight: FontWeight.w600,
              ),
            ),
          ),
          TextButton(onPressed: onViewTap ?? () {}, child: const Text('Xem')),
        ],
      ),
    );
  }
}
