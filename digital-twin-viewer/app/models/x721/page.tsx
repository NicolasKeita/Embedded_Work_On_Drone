import type { Metadata } from 'next';
import { X721Studio } from './x721-studio';

export const metadata: Metadata = {
  title: 'X721 — Étude de forme 3D',
  description: 'Modèle visuel exploratoire du drone X721, réalisé à partir des images de référence.',
};

export default function X721ModelPage() {
  return <X721Studio />;
}
