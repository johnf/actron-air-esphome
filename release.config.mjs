/**
 * @type {import('semantic-release').GlobalConfig}
 */
export default {
  plugins: [
    [
      '@semantic-release/commit-analyzer',
      {
        preset: 'conventionalcommits',
      },
    ],
    [
      '@semantic-release/release-notes-generator',
      {
        preset: 'conventionalcommits',
      },
    ],
    '@semantic-release/changelog',
    '@semantic-release/github',
    [
      '@semantic-release/exec',
      {
        prepareCmd:
          // biome-ignore lint/suspicious/noTemplateCurlyInString: Syntax for semantic-release
          'node scripts/update-manifest-version.mjs ${nextRelease.version}',
      },
    ],
    [
      '@semantic-release/git',
      {
        assets: ['CHANGELOG.md', 'package.json', 'manifest.json'],
      },
    ],
  ],
};
